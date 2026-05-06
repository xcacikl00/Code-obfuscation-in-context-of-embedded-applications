from pathlib import Path
import capstone
from sklearn.feature_extraction.text import CountVectorizer
from sklearn.preprocessing import StandardScaler
import numpy as np
from elftools.elf.elffile import ELFFile
import math
import collections
import angr
from tqdm import tqdm
import re
from sklearn.model_selection import train_test_split, GridSearchCV, StratifiedKFold
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report
import xgboost as xgb
import os
import joblib
import sys

OUTPUT_CLEAN = "./dataset/clean"
OUTPUT_OBFUSCATED = "./dataset/obfuscated"

def predict_obfuscation(elf_path, rf_path= "rf_obfuscation_model.joblib", xgb_path = "xgb_obfuscation_model.joblib", scaler_path="scaler.joblib", vectorizer_path="vectorizer.joblib"):
    rf_model = joblib.load(rf_path)
    xgb_model = joblib.load(xgb_path)
    scaler = joblib.load(scaler_path)
    vectorizer = joblib.load(vectorizer_path)

    # extract raw features
    entropy = calculate_text_section_entropy(elf_path)
    bin_mnemonics = get_mnemonics(elf_path)
    instruction_entropy = calculate_instruction_entropy(bin_mnemonics)
    graph_features = get_graph_features(elf_path)

    if graph_features[0] == 0:
        return None


    ngram_matrix = vectorizer.transform([bin_mnemonics]).toarray()
    
    # build dense features
    dense_features = [entropy, instruction_entropy] + graph_features
    dense_matrix = np.array([dense_features])

    # combine and scale
    final_vector = np.hstack((ngram_matrix, dense_matrix))
    scaled_vector = scaler.transform(final_vector)

    # predict
    rf_pred = rf_model.predict(scaled_vector)[0]
    xgb_pred = xgb_model.predict(scaled_vector)[0]

    return {
        "rf_prediction": "obfuscated" if rf_pred == 1 else "clean",
        "xgb_prediction": "obfuscated" if xgb_pred == 1 else "clean"
    }

def calculate_text_section_entropy(file_path, window_size=256):
    with open(file_path, "rb") as f:
        elf = ELFFile(f)
        section = elf.get_section_by_name(".text")
        data = section.data()

    if len(data) < window_size:
        return shannon(data)

    max_h = 0.0
    for i in range(len(data) - window_size + 1):
        h = shannon(data[i : i + window_size])
        if h > max_h:
            max_h = h
    return max_h

def shannon(data):
    counts = collections.Counter(data)
    total = len(data)
    return -sum((count / total) * math.log2(count / total) for count in counts.values())

def calculate_instruction_entropy(mnemonics_str):

    opcodes = mnemonics_str.split()

    counts = collections.Counter(opcodes)
    total = len(opcodes)
    h_op = 0.0
    
    for count in counts.values():
        p_op = count / total
        h_op -= p_op * math.log2(p_op)
        
    return h_op



# disassemble and normalize

def get_mnemonics(binary_path):
    engine = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
    
    with open(binary_path, "rb") as f:
        elf = ELFFile(f)
        code_section = elf.get_section_by_name('.text')
        
        if not code_section:
            return ""
            
        # Get the raw bytes and the virtual address
        code = code_section.data()
        address = code_section['sh_addr']
    
    return " ".join([i.mnemonic for i in engine.disasm(code, address)])


def get_graph_features(binary_path):
    proj = angr.Project(binary_path, auto_load_libs=False)
    cfg = proj.analyses.CFGFast()
    
    num_nodes = cfg.graph.number_of_nodes()
    num_edges = cfg.graph.number_of_edges()
    if num_nodes == 0:
        # print(f"no nodes for {binary_path}")
        return [0, 0, 0, 0]
    global_cyclomatic_complexity = num_edges - num_nodes + 2
    in_degrees = dict(cfg.graph.in_degree())
    max_in_degree = max(in_degrees.values()) if in_degrees.__len__() > 0 else 0

    
        
    total_instructions = 0
    for node in cfg.graph.nodes():
        try:
            if node.block and node.block.size > 0:
                total_instructions += node.block.instructions
        except (angr.errors.SimTranslationError):
            print(f"Error processing binary {binary_path}")
            # skip blocks that cannot be processed
            continue
    mean_bb_instructions = total_instructions / num_nodes
    edge_to_node_ratio = num_edges / num_nodes
    
    return [global_cyclomatic_complexity, max_in_degree, mean_bb_instructions, edge_to_node_ratio]


def save_vector_data(final_vector, final_labels, file_path="./final_vector.npz"):

    print(f"Saving final_vector to {file_path}...")
    np.savez(file_path, vector=final_vector, labels=np.array(final_labels))
    print(f"Data saved successfully.")


def load_vector_data(file_path="./final_vector.npz"):

    if Path(file_path).exists():
        print(f"Loading final_vector from {file_path}...")
        data = np.load(file_path, allow_pickle=True)
        final_vector = data['vector']
        final_labels = data['labels'].tolist()
        # Load binary_paths if available (for function-level splitting)
        binary_paths = data['binary_paths'].tolist() if 'binary_paths' in data.files else None
        print(f"Data loaded successfully. Shape: {final_vector.shape}")
        return final_vector, final_labels, binary_paths
    return None, None, None


def extract_base_function_name(binary_path):
    # removes suffixes from dataset_builder for function level splitting

    filename = Path(binary_path).stem  # remove extension
    base = re.sub(r'_-O[0-9s].*$', '', filename)
    base = re.sub(r'_-Ofast.*$', '', base)
    return base


def split_by_source_function(binary_paths, labels, features, test_size=0.15, val_size=0.15, seed=42):
    # split dataset at source function level to prevent data leakage.

    function_names = np.array([extract_base_function_name(bp) for bp in binary_paths])
    unique_functions = sorted(list(set(function_names)))
    
    # Split function to get the test set 15%

    funcs_train_val, funcs_test = train_test_split(
        unique_functions, 
        test_size=test_size, 
        random_state=seed
    )
    
    # split the 85% into Train and Val

    relative_val_size = val_size / (1 - test_size)
    funcs_train, funcs_val = train_test_split(
        funcs_train_val, 
        test_size=relative_val_size, 
        random_state=seed
    )
    
    #  sets for performance
    train_set = set(funcs_train)
    val_set = set(funcs_val)
    test_set = set(funcs_test)

    # map function names back to original indexes
    train_idx = [i for i, f in enumerate(function_names) if f in train_set]
    val_idx = [i for i, f in enumerate(function_names) if f in val_set]
    test_idx = [i for i, f in enumerate(function_names) if f in test_set]

    X_train, y_train = features[train_idx], np.array(labels)[train_idx]
    X_val, y_val = features[val_idx], np.array(labels)[val_idx]
    X_test, y_test = features[test_idx], np.array(labels)[test_idx]

    print(f"splits: train={len(train_set)} funcs, val={len(val_set)} funcs, test={len(test_set)} funcs")
    
    return X_train, X_val, X_test, y_train, y_val, y_test, train_set, val_set, test_set

def extract_features_by_prefix(prefix, output_clean="./dataset/clean", output_obfuscated="./dataset/obfuscated"):
    
    #debug function, prints features for all binaries with the same name prefix 
    # used to manually see the effects of flags and obfuscation on the data point

    clean_path = Path(output_clean)
    obf_path = Path(output_obfuscated)

    clean_files = list(clean_path.glob(f"{prefix}*"))
    obf_files = list(obf_path.glob(f"{prefix}*"))
    
    all_files = clean_files + obf_files
    print("="*40)

    if not all_files:
        print(f"no files found with {prefix} prefix")
        return
    
    print(f"Extracting features for files with prefix: {prefix}")
    
    for binary_file in all_files:
        print(f"File: {binary_file.name} ")
        entropy = calculate_text_section_entropy(binary_file)
        mnemonics = get_mnemonics(binary_file)
        instruction_entropy = calculate_instruction_entropy(mnemonics)
        graph_features = get_graph_features(binary_file)
        
        print(f"text section entropy: {entropy:.4f}")
        print(f"instruction entropy: {instruction_entropy:.4f}")
        print(f" global cyclomatic complexity: {graph_features[0]}")
        print(f" max in degree: {graph_features[1]}")
        print(f" Mean basic block instructions: {graph_features[2]:.4f}")
        print(f" edge to node ratio: {graph_features[3]:.4f}")
        print("="*40)
        




if __name__ == "__main__":
    os.chdir(Path(__file__).resolve().parent)
    
    if len(sys.argv) == 2:
        print(predict_obfuscation(sys.argv[1]))
        exit()
        


    VECTOR_FILE = "./final_vector.npz"
    
    # Try to load existing data
    loaded_data_result = load_vector_data(VECTOR_FILE)
    
    if loaded_data_result[0] is not None:
        final_vector, final_labels, binary_paths = loaded_data_result
    else:
        binaries = [] 

        clean_files = list(Path(OUTPUT_CLEAN).rglob("*.elf"))
        obfuscated_files = list(Path(OUTPUT_OBFUSCATED).rglob("*.elf"))
        
        binaries = clean_files + obfuscated_files
        final_labels = []
        binary_paths = []

        mnemonic_string_arr =[]
        dense_feature_matrix = []
        print(f"extracting features from {len(binaries)} binaries. ")

        for bin in tqdm(binaries):  
            entropy = calculate_text_section_entropy(bin)
            bin_mnemonics = get_mnemonics(bin)
            
            instruction_entropy = calculate_instruction_entropy(bin_mnemonics)
            graph_features = get_graph_features(bin)
            if graph_features[0] == 0: # contains no basic blocks, failed compilation, skip
                continue
            mnemonic_string_arr.append(bin_mnemonics)
            dense_feature_matrix.append([entropy, instruction_entropy] + graph_features)
            binary_paths.append(str(bin))

            if "dataset/clean" in str(bin):
                final_labels.append(0)
            else:
                final_labels.append(1)
            

        vectorizer = CountVectorizer(ngram_range=(1, 2), max_features=50)
        ngram_matrix = vectorizer.fit_transform(mnemonic_string_arr).toarray()
        joblib.dump(vectorizer, "./vectorizer.joblib")
        # concatenate and scale
        final_vector = np.hstack((ngram_matrix, dense_feature_matrix))
        
        # save with binary paths for function-level splitting
        print(f"Saving final_vector")
        np.savez("./final_vector.npz", vector=final_vector, labels=np.array(final_labels), 
                 binary_paths=np.array(binary_paths, dtype=object))


    scaler = StandardScaler()
    scaled_features = scaler.fit_transform(final_vector)
    print("final feature vector dimensions:", scaled_features.shape)
    
    extract_features_by_prefix("basic_math_large", OUTPUT_CLEAN, OUTPUT_OBFUSCATED)


    print("MODEL TRAINING AND EVALUATION")


    #  partitioning at function level (70% train, 15% val, 15% test) 
    X_train, X_val, X_test, y_train, y_val, y_test, train_funcs, val_funcs, test_funcs = split_by_source_function(
        binary_paths, final_labels, scaled_features, test_size=0.15, val_size=0.15, seed=42
    )


    
    rf_param_grid = {
        'n_estimators': [100, 200, 500],
        'max_features': ['sqrt', 'log2']
    }
    
    xgb_param_grid = {
        'learning_rate': [0.01, 0.1, 0.2],
        'gamma': [0, 0.1, 0.2]
    } 


    print("\ndataset partitioning:")
    print(f"training split functions: {len(train_funcs)} unique functions, {len(X_train)} binary variants ({len(X_train)/len(scaled_features)*100:.1f}%)")
    print(f"validation split functions: {len(val_funcs)} unique functions, {len(X_val)} binary variants ({len(X_val)/len(scaled_features)*100:.1f}%)")
    print(f"test split functions: {len(test_funcs)} unique functions, {len(X_test)} binary variants ({len(X_test)/len(scaled_features)*100:.1f}%)")
    print(f"\n total unique functions: {len(train_funcs) + len(val_funcs) + len(test_funcs)}")
    print(f" total samples: {len(X_train) + len(X_val) + len(X_test)}")
    
    rf_model = RandomForestClassifier(
        max_depth=15, 
        min_samples_leaf=5, 
        random_state=42
    )

    rf_grid_search = GridSearchCV(
        rf_model, rf_param_grid, cv=StratifiedKFold(n_splits=5, shuffle=True, random_state=42),
        n_jobs=-1, verbose=1, scoring='accuracy'
    )
    rf_grid_search.fit(X_train, y_train)
    
    
    print(f"\nBest Random Forest Parameters: {rf_grid_search.best_params_}")
    print(f"Best Cross-Validation Score (5-Fold): {rf_grid_search.best_score_:.4f}")
    
    #  evaluate on all sets
    rf_train_score = rf_grid_search.score(X_train, y_train)
    rf_val_score = rf_grid_search.score(X_val, y_val)
    rf_test_score = rf_grid_search.score(X_test, y_test)
    
    print("\nRF performance:\n")
    print(f"training accuracy: {rf_train_score:.4f}")
    print(f"validation accuracy: {rf_val_score:.4f}")
    print(f"test accuracy: {rf_test_score:.4f}")
    
    y_pred_rf = rf_grid_search.predict(X_test)
    print(f"\n RF report:")
    print(classification_report(y_test, y_pred_rf, target_names=['Clean', 'Obfuscated']))
    

    xgb_model = xgb.XGBClassifier(
        random_state=42, 
        eval_metric='logloss',
        reg_lambda=1.0,           # l2 regularization
    )

    xgb_grid_search = GridSearchCV(
        xgb_model, xgb_param_grid, cv=StratifiedKFold(n_splits=5, shuffle=True, random_state=42),
        n_jobs=-1, verbose=1, scoring='accuracy'
    )

    xgb_grid_search.fit(X_train, y_train)

            
    # xboost eval
    xgb_train_score = xgb_grid_search.score(X_train, y_train)
    xgb_val_score = xgb_grid_search.score(X_val, y_val)
    xgb_test_score = xgb_grid_search.score(X_test, y_test)
    
    print("\nxgboost performance:")
    print(f"training accuracy: {xgb_train_score:.4f}")
    print(f"validation accuracy: {xgb_val_score:.4f}")
    print(f"test accuracy: {xgb_test_score:.4f}")
    
    y_pred_xgb = xgb_grid_search.predict(X_test)
    print("\nxgboost report:")
    print(classification_report(y_test, y_pred_xgb, target_names=['Clean', 'Obfuscated']))
    # dump created models 
    joblib.dump(rf_grid_search.best_estimator_, 'rf_obfuscation_model.joblib')
    joblib.dump(xgb_grid_search.best_estimator_, 'xgb_obfuscation_model.joblib')
    joblib.dump(scaler, 'scaler.joblib')

    




