int bit_str_size = 4;
int num_qubits = bit_str_size + 1;

def superpose(qubit[num_qubits] qubits, int bit_str_size) {
    for int i in [0:bit_str_size - 1] {
        h qubits[i];
    }
}

def unitary(qubit[num_qubits] qubits, int size, readonly array[int, #dim = 1] bit_string, int bit_string_size) {
    x qubits[size - 1];
    h qubits[size - 1];

    for int i in [0:bit_string_size - 1] {
        if (bit_string[i] == 1) {
            cx qubits[i], qubits[size - 1];
        }
    }
}

def readout(mutable array[int, #dim = 1] measurements, int bit_str_size, qubit[num_qubits] qubits) {
    for int i in [0:bit_str_size - 1] {
        measurements[i] = measure qubits[i];
    }
}

int num_shots = 10;
qubit[num_qubits] qubits;
array[int, bit_str_size] bit_string = {1, 0, 1, 1};
output array[int, num_shots, bit_str_size] measurements;

for int i in [0:num_shots - 1] {
    reset qubits;
    superpose(qubits, bit_str_size);
    unitary(qubits, num_qubits, bit_string, bit_str_size);
    superpose(qubits, bit_str_size);
    readout(measurements[i], bit_str_size, qubits);
}
