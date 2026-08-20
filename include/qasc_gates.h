/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#pragma once

#include "qasc_core.h"

#include "quest.h"

#include <numbers>
#include <utility>

namespace qasc {

inline Qureg qascInternalQuestQureg;
inline bool isQascInternalQuestQuregInit{false};

inline bool shouldPrint() {
    return qascInternalQuestQureg.rank == 0;
}

// Used by tests
inline void resetQascEnv() {
    destroyQureg(qascInternalQuestQureg);
    isQascInternalQuestQuregInit = false;
    nextQuregStartIdx = 0;
}

template <QasmQuregT T>
void x(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliX(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void X(T&& q) {
    x(std::forward<T>(q));
}

template <QasmQuregT T>
void y(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliY(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void z(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliZ(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void h(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyHadamard(qascInternalQuestQureg,
                      getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void s(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyS(qascInternalQuestQureg,
               getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void sdg(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i),
                        -std::numbers::pi / 2.0);
    }
}

template <QasmQuregT T>
void sx(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateX(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i),
                     std::numbers::pi / 2.0);
    }
}

template <QasmQuregT T>
void t(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyT(qascInternalQuestQureg,
               getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void tdg(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i),
                        -std::numbers::pi / 4.0);
    }
}

void gphase(double theta) {
    for (int i = 0; i < qascInternalQuestQureg.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg, i, theta);
    }
}

template <QasmQuregT T>
void U(double theta, double phi, double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), phi);
        applyRotateY(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), theta);
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), lambda);
    }
}

template <QasmQuregT T>
void u1(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), lambda);
    }
}

template <QasmQuregT T>
void u2(double phi, double lambda, T&& q) {
    gphase(-(phi + lambda + std::numbers::pi / 2.0) / 2.0);
    U(std::numbers::pi / 2.0, phi, lambda, std::forward<T>(q));
}

template <QasmQuregT T>
void u3(double theta, double phi, double lambda, T&& q) {
    gphase(-(phi + lambda + theta) / 2.0);
    U(theta, phi, lambda, std::forward<T>(q));
}

template <QasmQuregT T>
void p(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), lambda);
    }
}

template <QasmQuregT T>
void phase(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), lambda);
    }
}

template <QasmQuregT T>
void rx(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateX(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), theta);
    }
}

template <QasmQuregT T>
void ry(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateY(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), theta);
    }
}

template <QasmQuregT T>
void rz(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateZ(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cx(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliX(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), i),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void CX(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliX(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cX(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliX(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), i),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cy(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliY(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cz(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliZ(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void ch(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledHadamard(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T1>(c), 0),
                                getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cs(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledS(qascInternalQuestQureg,
                         getGlobalQubitIdx(std::forward<T1>(c), 0),
                         getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void ct(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledT(qascInternalQuestQureg,
                         getGlobalQubitIdx(std::forward<T1>(c), 0),
                         getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cu(double theta, double phi, double lambda, double gamma, T1&& c, T2&& t) {
    p(c, lambda - gamma / 2.0);
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T1>(c), 0),
                                getGlobalQubitIdx(std::forward<T2>(t), i), phi);
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T2>(t), i), gamma);
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
        applyTwoQubitPhaseShift(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T2>(t), i),
                                lambda);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cU(double theta, double phi, double lambda, double gamma, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T1>(c), 0),
                                getGlobalQubitIdx(std::forward<T2>(t), i), phi);
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T2>(t), i), gamma);
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), lambda);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cp(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void swap(T1&& a, T2&& b) {
    for (int i = 0; i < a.numQubits; ++i) {
        applySwap(qascInternalQuestQureg,
                  getGlobalQubitIdx(std::forward<T1>(a), i),
                  getGlobalQubitIdx(std::forward<T2>(b), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2, QasmQuregT T3>
void cswap(T1&& c, T2&& a, T3&& b) {
    for (int i = 0; i < a.numQubits; ++i) {
        applyControlledSwap(qascInternalQuestQureg,
                            getGlobalQubitIdx(std::forward<T1>(c), 0),
                            getGlobalQubitIdx(std::forward<T2>(a), i),
                            getGlobalQubitIdx(std::forward<T3>(b), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2, QasmQuregT T3>
void ccx(T1&& c1, T2&& c2, T3&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyMultiControlledPauliX(qascInternalQuestQureg,
                                   {getGlobalQubitIdx(std::forward<T1>(c1), 0),
                                    getGlobalQubitIdx(std::forward<T2>(c2), 0)},
                                   getGlobalQubitIdx(std::forward<T3>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cphase(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void crx(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateX(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void cry(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void crz(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateZ(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), theta);
    }
}

template <QasmQuregT T>
void reset(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        int res = applyQubitMeasurement(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T>(q), i));
        if (res == 1) {
            applyPauliX(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i));
        }
    }
}

template <QasmQuregT T>
std::vector<unsigned int> measure(T&& q) {
    if (q.start < 0 || q.end < 0 || q.increment < 0) {
        throw std::runtime_error("negative indexing not supported");
    }

    std::vector<unsigned int> result(q.numQubits);
    for (int i = 0; i < q.numQubits; ++i) {
        result[i] = applyQubitMeasurement(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T>(q), i));
    }
    return result;
}

template <QasmQuregT T>
unsigned int one_qubit_measure(T&& q) {
    if (q.numQubits != 1) {
        throw std::runtime_error(
            "error: attempting to do single qubit measurement on multiqubit "
            "quantum register.");
    }

    return applyQubitMeasurement(qascInternalQuestQureg,
                                 getGlobalQubitIdx(std::forward<T>(q), 0));
}
// inverse gates

template <QasmQuregT T>
void inv_x(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliX(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void inv_X(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliX(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void inv_y(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliY(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void inv_z(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPauliZ(qascInternalQuestQureg,
                    getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void inv_h(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyHadamard(qascInternalQuestQureg,
                      getGlobalQubitIdx(std::forward<T>(q), i));
    }
}

template <QasmQuregT T>
void inv_s(T&& q) {
    sdg(std::forward<T>(q));
}

template <QasmQuregT T>
void inv_sdg(T&& q) {
    s(std::forward<T>(q));
}

template <QasmQuregT T>
void inv_sx(T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateX(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i),
                     -std::numbers::pi / 2.0);
    }
}

template <QasmQuregT T>
void inv_t(T&& q) {
    tdg(std::forward<T>(q));
}

template <QasmQuregT T>
void inv_tdg(T&& q) {
    t(std::forward<T>(q));
}

inline void inv_gphase(double theta) {
    for (int i = 0; i < qascInternalQuestQureg.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg, i, -theta);
    }
}

template <QasmQuregT T>
void inv_U(double theta, double phi, double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), -lambda);
        applyRotateY(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), -theta);
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), -phi);
    }
}

template <QasmQuregT T>
void inv_u1(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), -lambda);
    }
}

template <QasmQuregT T>
void inv_u2(double phi, double lambda, T&& q) {
    U(-std::numbers::pi / 2.0, -phi, -lambda, std::forward<T>(q));
    gphase((phi + lambda + std::numbers::pi / 2.0) / 2.0);
}

template <QasmQuregT T>
void inv_u3(double theta, double phi, double lambda, T&& q) {
    U(-theta, -phi, -lambda, std::forward<T>(q));
    gphase((phi + lambda + theta) / 2.0);
}

template <QasmQuregT T>
void inv_p(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), -lambda);
    }
}

template <QasmQuregT T>
void inv_phase(double lambda, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T>(q), i), -lambda);
    }
}

template <QasmQuregT T>
void inv_rx(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateX(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), -theta);
    }
}

template <QasmQuregT T>
void inv_ry(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateY(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), -theta);
    }
}

template <QasmQuregT T>
void inv_rz(double theta, T&& q) {
    for (int i = 0; i < q.numQubits; ++i) {
        applyRotateZ(qascInternalQuestQureg,
                     getGlobalQubitIdx(std::forward<T>(q), i), -theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cx(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliX(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), i),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_CX(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliX(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cy(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliY(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cz(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPauliZ(qascInternalQuestQureg,
                              getGlobalQubitIdx(std::forward<T1>(c), 0),
                              getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_ch(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledHadamard(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T1>(c), 0),
                                getGlobalQubitIdx(std::forward<T2>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cs(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPhaseGadget(qascInternalQuestQureg,
                                   getGlobalQubitIdx(std::forward<T1>(c), i),
                                   {getGlobalQubitIdx(std::forward<T2>(t), 1)},
                                   -std::numbers::pi / 2.0);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_ct(T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledPhaseGadget(qascInternalQuestQureg,
                                   getGlobalQubitIdx(std::forward<T1>(c), i),
                                   {getGlobalQubitIdx(std::forward<T2>(t), 1)},
                                   -std::numbers::pi / 4.0);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cu(double theta,
            double phi,
            double lambda,
            double gamma,
            T1&& c,
            T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T2>(t), i),
                                -lambda);
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);

        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T2>(t), i), -gamma);

        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -phi);
    }
    inv_p(std::forward<T1>(c), (gamma - theta / 2.0));
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cU(double theta,
            double phi,
            double lambda,
            double gamma,
            T1&& c,
            T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(qascInternalQuestQureg,
                                getGlobalQubitIdx(std::forward<T2>(t), i),
                                -lambda);
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);

        applyPhaseShift(qascInternalQuestQureg,
                        getGlobalQubitIdx(std::forward<T2>(t), i), -gamma);

        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -phi);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cp(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_swap(T1&& a, T2&& b) {
    for (int i = 0; i < a.numQubits; ++i) {
        applySwap(qascInternalQuestQureg,
                  getGlobalQubitIdx(std::forward<T1>(a), i),
                  getGlobalQubitIdx(std::forward<T2>(b), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2, QasmQuregT T3>
void inv_cswap(T1 c, T2 a, T3 b) {
    for (int i = 0; i < a.numQubits; ++i) {
        applyControlledSwap(qascInternalQuestQureg,
                            getGlobalQubitIdx(std::forward<T1>(c), 0),
                            getGlobalQubitIdx(std::forward<T2>(a), i),
                            getGlobalQubitIdx(std::forward<T3>(b), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2, QasmQuregT T3>
void inv_ccx(T1&& c1, T2&& c2, T3&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyMultiControlledPauliX(qascInternalQuestQureg,
                                   {getGlobalQubitIdx(std::forward<T1>(c1), 0),
                                    getGlobalQubitIdx(std::forward<T2>(c2), 0)},
                                   getGlobalQubitIdx(std::forward<T3>(t), i));
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cphase(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyTwoQubitPhaseShift(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_crx(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateX(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_cry(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateY(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);
    }
}

template <QasmQuregT T1, QasmQuregT T2>
void inv_crz(double theta, T1&& c, T2&& t) {
    for (int i = 0; i < t.numQubits; ++i) {
        applyControlledRotateZ(
            qascInternalQuestQureg, getGlobalQubitIdx(std::forward<T1>(c), 0),
            getGlobalQubitIdx(std::forward<T2>(t), i), -theta);
    }
}
}    // namespace qasc
