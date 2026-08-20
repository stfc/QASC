int num_shots = 10;
int num_qubits = 4;
qubit[num_qubits] qubits;
output array[int, num_shots, num_qubits] measurements;

for int shot in [0 : num_shots - 1] {
    reset qubits;
    for int i in [0 : num_qubits - 1] {
        h qubits[i];

        float param = pi / 2;
        for int j in [i + 1 : num_qubits - 1] {
            crz(param) qubits[j], qubits[i]; 
            param /= 2;
        }

    }

    for int i in [0 : num_qubits / 2 - 1] {
        int j = num_qubits - (i + 1);
        swap qubits[i], qubits[j];
    }

    for int i in [0 : num_qubits - 1] {
         measurements[shot][i] = measure qubits[i];
    }
}
