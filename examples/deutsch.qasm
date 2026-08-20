int num_qubits = 2;
int num_shots = 10;
output array[int, num_shots] measurements;
qubit[num_qubits] qubits;

for int i in [0 : num_shots - 1] {
    reset qubits;
    h qubits[0];
    x qubits[1];
    h qubits[1];
    cx qubits[0], qubits[1];
    x qubits[1];
    h qubits[0];
    measurements[i] = measure qubits[0];
}
