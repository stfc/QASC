/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "include/qasc_core.h"
#include "include/qasc_gates.h"

#include "config_paths.h"
#include "src/utils.h"

#include "doctest.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <numbers>
#include <string>
#include <vector>

using namespace qasc;

namespace {
constexpr double testEps = 1e-6;

}

void initQascEnv(int numQubits) {
    if (!isQuESTEnvInit()) {
        initQuESTEnv();
        // setSeeds({42}); // gcc complains...
        unsigned seeds[] = {42};
#if defined(DEPS_QUEST_VERSION_MINOR) && (DEPS_QUEST_VERSION_MINOR >= 3)
        setQuESTSeeds(seeds, 1);
#else
        setSeeds(seeds, 1);
#endif
    }
    if (!isQascInternalQuestQuregInit) {
        isQascInternalQuestQuregInit = true;
        qascInternalQuestQureg = createQureg(numQubits);
    };
}

TEST_CASE("testing for loops") {
    SUBCASE("for loop with classical register (of bits)") {
        std::string bitstr = "010101";
        QasmType<unsigned int, 6> reg(bitstr);
        CHECK(reg == 21);
        int i = 0;

        // note bits inside QasmType are reversed
        std::reverse(bitstr.begin(), bitstr.end());

        for (auto& b : reg) {
            CHECK(b == (bitstr[i] == '0' ? 0 : 1));
            ++i;
        }
    }
}

TEST_CASE("testing while loops") {
    const auto numQubits = 1;
    auto q = QuregSlice(numQubits);
    initQascEnv(numQubits);

    QasmType<int, 32> x(0);
    int i = 0;
    constexpr int maxIter = 100000;
    while (i < maxIter) {
        h(q);
        QasmType<unsigned int, 1> res(measure(q));
        if (res) {
            x += 1;
        }
        ++i;
    }

    CHECK(std::abs(static_cast<double>(x / static_cast<double>(maxIter)) -
                   0.5) < 0.1);
    resetQascEnv();
}

namespace {
template <QasmQuregT T>
QasmType<unsigned int, 1> fn1(QasmType<int, 32> i, T d) {
    return measure(d[i]);
}

template <QasmQuregT T>
QasmType<int, 32> fn2(T q) {
    QasmType<int, 32> r(0);
    QasmType<unsigned int, 1> k(0);
    x(q);
    for (auto i = 0; i < 7 + 1; i += 1) {
        k = measure(q[i]);
        r += k;
    }
    return r;
}

}    // namespace

TEST_CASE("testing switch statement.") {
    SUBCASE("generated switch with multiple cases per case-body compiles.") {
        QasmType<int, 32> opt = 42;
        switch (opt) {
            case 4: {
                {
                    opt += 5;
                }
                break;
            }
            case 2: {
                {
                    opt += 5;
                }
                break;
            }
            case 6: {
                {
                    opt += 5;
                }
                break;
            }
            case -4: {
                {
                    opt -= 5;
                }
                break;
            }
            case -2: {
                {
                    opt -= 5;
                }
                break;
            }
            case -6: {
                {
                    opt -= 5;
                }
                break;
            }
            case 9: {
                {
                    opt += 9;
                }
                break;
            }
            case 11: {
                {
                    opt += 9;
                }
                break;
            }
            case 42: {
                {
                    opt += 9;
                }
                break;
            }
            default: {
                {
                    opt -= 42;
                }
                break;
            }
        }

        CHECK(opt == 51);
    }

    SUBCASE("generated switch with cases as variables compiles.") {
        constexpr QasmType<int, 32> A(-3);
        constexpr QasmType<int, 32> B(21);
        QasmType<int, 32> opt(19);
        switch (opt) {
            case A: {
                {
                    opt += 3;
                }
                break;
            }
            case B - 2: {
                {
                    opt += 4;
                }
                break;
            }
            case B + 3: {
                {
                    opt += 5;
                }
                break;
            }
            default: {
                {
                    opt += 6;
                }
                break;
            }
        }

        CHECK(opt == 23);
    }

    SUBCASE("generated switch on binary casted to int compiles.") {
        QasmType<int, 32> res(0);
        QasmType<unsigned int, 2> i(0);
        switch (static_cast<QasmType<int>>(i)) {
            case 0b00: {
                {
                    res += 3;
                }
                break;
            }
            case 0b01: {
                {
                    res += 4;
                }
                break;
            }
            case 0b10: {
                {
                    res += 5;
                }
                break;
            }
            case 0b11: {
                {
                    res += 6;
                }
                break;
            }
            default: {
                {
                    res += 7;
                }
                break;
            }
        }

        CHECK(res == 3);
    }

    SUBCASE(
        "generated switch with complex cases calling quantum functions "
        "compiles.") {
        QasmType<int, 32> cond(1);
        QasmType<int, 32> res(1);
        QasmType<int, 32> offset(2);
        QasmType<unsigned int, 1> c1(0);
        const auto numQubits = 8;
        auto q0 = QuregSlice(numQubits);
        initQascEnv(numQubits);

        switch (cond) {
            case 1: {
                {
                    res = offset + fn1(offset, q0);
                }
                break;
            }
            case 2: {
                {
                    double d = res / offset;
                }
                break;
            }
            case 3: {
                {
                }
                break;
            }
            default: {
                {
                }
                break;
            }
        }
        CHECK(res == 2);
        resetQascEnv();
    }
    SUBCASE("generated nested switch compiles.") {
        const auto numQubits = 8;
        auto q = QuregSlice(numQubits);
        initQascEnv(numQubits);

        QasmType<int, 32> cond1(15);
        QasmType<int, 32> cond2(fn2(q));
        int res = 0;
        switch (cond2) {
            case 1: {
                {
                }
                break;
            }
            case 2: {
                {
                }
                break;
            }
            case 5: {
                {
                }
                break;
            }
            case 12: {
                {
                }
                break;
            }
            case 8: {
                {
                    switch (cond1) {
                        case 10: {
                            {
                                h(q);
                            }
                            break;
                        }
                        case 15: {
                            {
                                h(q);
                                res = 22;
                            }
                            break;
                        }
                        case 20: {
                            {
                                h(q);
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
        CHECK(res == 22);
        resetQascEnv();
    }
}

TEST_CASE("testing generated gate calls compile and work.") {
    SUBCASE("single qubit gates") {
        const auto numQubits = 8;
        auto q = QuregSlice(numQubits);
        SUBCASE("x") {
            initQascEnv(numQubits);
            x(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("y") {
            initQascEnv(numQubits);
            h(q);
            y(q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("z") {
            initQascEnv(numQubits);
            h(q);
            z(q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("s") {
            initQascEnv(numQubits);
            h(q);
            z(q);
            s(q);
            s(q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 0);
            }
            resetQascEnv();
        }
        SUBCASE("sdg") {
            initQascEnv(numQubits);
            h(q);
            s(q);
            sdg(q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 0);
            }
            resetQascEnv();
        }
        SUBCASE("sx") {
            initQascEnv(numQubits);
            sx(q);
            sx(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("t") {
            initQascEnv(numQubits);
            h(q);
            z(q);
            for (auto i = 0; i < 4; ++i) {
                t(q);
            }
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 0);
            }
            resetQascEnv();
        }
        SUBCASE("tdg") {
            initQascEnv(numQubits);
            h(q);
            t(q);
            tdg(q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 0);
            }
            resetQascEnv();
        }
    }

    SUBCASE("rotation  gates") {
        const auto numQubits = 1;
        auto q = QuregSlice(numQubits);
        SUBCASE("rx") {
            initQascEnv(numQubits);
            QasmType<unsigned int, 32, true> param1(std::numbers::pi);
            rx(param1, q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("ry") {
            initQascEnv(numQubits);
            std::vector<QasmType<unsigned int, 32, true>> params = {
                std::numbers::pi, std::numbers::pi};
            h(q);
            ry(params[0], q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("rz") {
            initQascEnv(numQubits);
            h(q);
            rz(std::numbers::pi, q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("p") {
            initQascEnv(numQubits);
            h(q);
            p(std::numbers::pi, q);
            h(q);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
    }

    SUBCASE("control gates") {
        const auto numQubits = 2;
        auto q = QuregSlice(numQubits);
        SUBCASE("cx") {
            initQascEnv(numQubits);
            x(q[0]);
            cx(q[0], q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("cy") {
            initQascEnv(numQubits);
            x(q[0]);
            h(q[1]);
            cy(q[0], q[1]);
            h(q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("cz") {
            initQascEnv(numQubits);
            x(q[0]);
            h(q[1]);
            cz(q[0], q[1]);
            h(q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("ch") {
            initQascEnv(numQubits);
            x(q[0]);
            ch(q[0], q[1]);
            ry(std::numbers::pi / 2.0, q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("swap") {
            initQascEnv(numQubits);
            x(q[0]);
            swap(q[0], q[1]);
            auto res = measure(q[1]);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
    }
    SUBCASE("controlled rotation") {
        const auto numQubits = 2;
        auto q = QuregSlice(numQubits);
        SUBCASE("crx") {
            initQascEnv(numQubits);
            x(q[0]);
            crx(std::numbers::pi, q[0], q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("cry") {
            initQascEnv(numQubits);
            x(q[0]);
            h(q[1]);
            cry(std::numbers::pi, q[0], q[1]);
            h(q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("crz") {
            initQascEnv(numQubits);
            x(q[0]);
            h(q[1]);
            crz(std::numbers::pi, q[0], q[1]);
            h(q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
        SUBCASE("cp") {
            initQascEnv(numQubits);
            x(q[0]);
            h(q[1]);
            cp(std::numbers::pi, q[0], q[1]);
            h(q[1]);
            auto res = measure(q);
            for (auto& v : res) {
                CHECK(v == 1);
            }
            resetQascEnv();
        }
    }

    SUBCASE("gphase") {
        const auto numQubits = 2;
        auto q = QuregSlice(numQubits);
        initQascEnv(numQubits);
        h(q);
        gphase(std::numbers::pi);
        h(q);
        auto res = measure(q);
        for (auto& v : res) {
            CHECK(v == 1);
        }
        resetQascEnv();
    }

    SUBCASE("on slice") {
        const auto numQubits = 6;
        auto q = QuregSlice(numQubits);
        initQascEnv(numQubits);
        x(getSlice(q, 1, 3, 1));
        auto res = measure(q);
        CHECK(res[0] == 0);
        CHECK(res[1] == 1);
        CHECK(res[2] == 1);
        CHECK(res[3] == 1);
        CHECK(res[4] == 0);
        CHECK(res[5] == 0);
        resetQascEnv();
    }
}

namespace {
template <QasmQuregT T>
void g1(T a) {
    x(a);
}

template <QasmQuregT T>
void g2(T a, T b) {
    x(b);
}

template <QasmQuregT T>
void g3(T a, T b, T c) {
    x(c);
}

template <QasmQuregT T>
void g4(T a, T b, T c, T d) {
    x(c);
    h(d);
    y(d);
    h(d);
}

template <QasmQuregT T>
void paramGate(double alpha, double beta, T a, T b) {
    rx(alpha, a);
    h(b);
    ry(beta, b);
    h(b);
}

template <QasmQuregT T>
void customCPhase(double theta, T a, T b) {
    U(0, 0, theta / 2, a);
    CX(a, b);
    U(0, 0, -theta / 2, b);
    CX(a, b);
    U(0, 0, theta / 2, b);
}

template <QasmQuregT T>
void customX(T a) {
    U(std::numbers::pi, 0, std::numbers::pi, a);
    gphase(-std::numbers::pi / 2.0);
}

template <QasmQuregT T>
void customCX(T a, T b) {
    cU(std::numbers::pi, 0, std::numbers::pi, 0, a, b);
    gphase(-std::numbers::pi / 2.0);
}
}    // namespace

TEST_CASE("testing gate definition.") {
    SUBCASE("applying different user defined gates.") {
        auto q1 = QuregSlice(1);
        auto q2 = QuregSlice(1);
        auto q3 = QuregSlice(1);
        initQascEnv(3);
        g1(q1);
        g2(q1, q2);
        g3(q1, q2, q3);
        auto res1 = measure(q1);
        auto res2 = measure(q2);
        auto res3 = measure(q3);

        CHECK(res1[0] == 1);
        CHECK(res2[0] == 1);
        CHECK(res3[0] == 1);

        resetQascEnv();
    }

    SUBCASE("applying same user defined gate to different qubits.") {
        auto q1 = QuregSlice(1);
        auto q2 = QuregSlice(2);
        auto q3 = QuregSlice(3);
        auto q4 = QuregSlice(3);
        initQascEnv(9);
        g4(q1[0], q2[0], q3[0], q4[0]);
        g4(q1[0], q2[1], q3[1], q4[0]);
        g4(q1[0], q2[0], q3[2], q4[1]);
        auto res1 = measure(q1);
        auto res2 = measure(q2);
        auto res3 = measure(q3);
        auto res4 = measure(q4);

        CHECK(res1[0] == 0);
        CHECK(res2[0] == 0);
        CHECK(res2[1] == 0);
        for (auto& m : res3) {
            CHECK(m == 1);
        }
        CHECK(res4[0] == 0);
        CHECK(res4[1] == 1);
        CHECK(res4[2] == 0);
        resetQascEnv();
    }

    SUBCASE("applying parameterised user defined gate.") {
        auto q = QuregSlice(2);
        initQascEnv(2);
        QasmType<unsigned int, 4, true> param1(std::numbers::pi);
        QasmType<unsigned int, 5, true> param2(std::numbers::pi);
        paramGate(param1, param2, q[0], q[1]);
        auto res = measure(q);

        CHECK(res[0] == 1);
        CHECK(res[1] == 1);

        resetQascEnv();
    }

    SUBCASE("testing custom controlled phase.") {
        auto q = QuregSlice(2);
        initQascEnv(2);
        QasmType<unsigned int, 32, true> theta(std::numbers::pi);
        x(q[0]);
        h(q[1]);
        customCPhase(theta, q[0], q[1]);
        cphase(theta, q[0], q[1]);
        h(q[1]);
        auto res = measure(q);

        CHECK(res[0] == 1);
        CHECK(res[1] == 0);

        resetQascEnv();
    }

    SUBCASE("testing custom x gate.") {
        auto q = QuregSlice(1);
        initQascEnv(1);
        x(q);
        customX(q);
        auto res = measure(q);

        CHECK(res[0] == 0);
        resetQascEnv();
    }

    SUBCASE("testing custom cx gate.") {
        auto q = QuregSlice(2);
        initQascEnv(2);
        x(q[0]);
        customCX(q[0], q[1]);
        auto res = measure(q);

        CHECK(res[0] == 1);
        CHECK(res[1] == 1);
        resetQascEnv();
    }
}

TEST_CASE("testing code generated by measure arrow assignment statements.") {
    SUBCASE("standard 1 qubit measurement") {
        const auto numQubits = 1;
        auto q = QuregSlice(numQubits);
        std::vector<unsigned int> res(1);
        initQascEnv(numQubits);
        x(q);
        res[0] = one_qubit_measure(q[0]);
        CHECK(res[0] == 1);

        resetQascEnv();
    }

    SUBCASE("measurement on sliced qureg") {
        const auto numQubits = 6;
        auto q = QuregSlice(numQubits);
        std::vector<unsigned int> res(4);
        initQascEnv(numQubits);
        x(getSlice(q, 0, 3, 1));
        res = measure(getSlice(q, 0, 3, 1));
        CHECK(res.size() == 4);
        for (auto& m : res) {
            CHECK(m == 1);
        }
        resetQascEnv();
    }

    SUBCASE("measurement on a more complex slice") {
        const auto numQubits = 6;
        auto q = QuregSlice(numQubits);
        std::vector<unsigned int> res(3);
        initQascEnv(numQubits);
        x(getSlice(q, 0, 4, 2));
        res = measure(getSlice(q, 0, 4, 2));
        CHECK(res.size() == 3);
        for (auto& m : res) {
            CHECK(m == 1);
        }

        resetQascEnv();
    }

    SUBCASE("assigning measurement result to a classical array slice") {
        const auto numQubits = 6;
        auto q = QuregSlice(numQubits);
        std::vector<unsigned int> res(6);
        initQascEnv(numQubits);
        x(q);
        setSlice(res, measure(q), 0, 3, 1);

        CHECK(res.size() == 6);
        for (auto i = 0; i < 4; ++i) {
            CHECK(res[i] == 1);
        }

        for (auto i = 4; i < res.size(); ++i) {
            CHECK(res[i] == 0);
        }

        resetQascEnv();
    }

    SUBCASE(
        "assigning measurement result to a more complex classical array "
        "slice") {
        const auto numQubits = 4;
        auto q = QuregSlice(numQubits);

        std::vector<unsigned int> res(25);
        initQascEnv(numQubits);

        x(q);
        setSlice(res, measure(q), 0, 9, 3);

        CHECK(res.size() == 25);
        for (auto i = 0; i < 10; i += 3) {
            CHECK(res[i] == 1);
            CHECK(res[i + 1] == 0);
            CHECK(res[i + 2] == 0);
        }

        for (auto i = 10; i < res.size(); ++i) {
            CHECK(res[i] == 0);
        }

        resetQascEnv();
    }

    SUBCASE("assigning measurement on qureg slice to classical array slice") {
        const auto numQubits = 4;
        auto q = QuregSlice(numQubits);
        std::vector<unsigned int> res(8);

        initQascEnv(numQubits);

        x(getSlice(q, 0, 3, 1));
        setSlice(res, measure(getSlice(q, 0, 3, 1)), 1, 4, 1);

        CHECK(res.size() == 8);

        CHECK(res[0] == 0);
        for (auto i = 1; i < 5; ++i) {
            CHECK(res[i] == 1);
        }

        for (auto i = 5; i < res.size(); ++i) {
            CHECK(res[i] == 0);
        }

        resetQascEnv();
    }
}

TEST_CASE("testing quantum register reset.") {
    SUBCASE("resetting whole qureg") {
        const auto numQubits = 4;
        auto q = QuregSlice(numQubits);

        initQascEnv(numQubits);

        x(q);
        reset(q);
        auto res = measure(q);
        for (auto& m : res) {
            CHECK(m == 0);
        }

        resetQascEnv();
    }
    SUBCASE("resetting slice on qureg") {
        const auto numQubits = 5;
        auto q = QuregSlice(numQubits);

        initQascEnv(numQubits);

        x(q);
        reset(getSlice(q, 1, 3, 1));
        auto res = measure(q);
        CHECK(res[0] == 1);
        CHECK(res[1] == 0);
        CHECK(res[2] == 0);
        CHECK(res[3] == 0);
        CHECK(res[4] == 1);

        resetQascEnv();
    }

    SUBCASE("resetting single qubit") {
        const auto numQubits = 4;
        auto q = QuregSlice(numQubits);

        initQascEnv(numQubits);

        x(q);
        reset(q[2]);
        auto res = measure(q);
        CHECK(res[0] == 1);
        CHECK(res[1] == 1);
        CHECK(res[2] == 0);
        CHECK(res[3] == 1);

        resetQascEnv();
    }
}

TEST_CASE("testing if statements.") {
    SUBCASE("generated one liner body without curly braces work.") {
        const auto numQubits = 1;
        auto q = QuregSlice(numQubits);
        initQascEnv(numQubits);

        int basis = 1;
        rx(-3.14159265358979323846 / 2, q);
        h(q);
        if (basis == 0)
            h(q);
        else if (basis == 1)
            rx(3.14159265358979323846 / 2, q);

        auto result = measure(q);
        CHECK(result[0] == 1);
        resetQascEnv();
    }
}

TEST_CASE("testing code generated by quantum aliasing.") {
    const auto numQubits = 12;
    auto q = QuregSlice(numQubits);
    initQascEnv(numQubits);
    auto first = QuregSlice(q, 0, 0, 1);
    CHECK(first.numQubits == 1);
    CHECK(first.start == 0);
    CHECK(first.end == 1);
    CHECK(getGlobalQubitIdx(first, 0) == 0);

    auto last = QuregSlice(q, -1, -1, 1);
    CHECK(last.numQubits == 1);
    CHECK(last.start == -1);
    CHECK(last.end == 0);
    CHECK(getGlobalQubitIdx(last, 0) == numQubits - 1);

    auto sliced = QuregSlice(q, 0, 8, 1);
    CHECK(sliced.numQubits == 9);
    CHECK(sliced.start == 0);
    CHECK(sliced.end == 9);
    for (auto i = 0; i < sliced.numQubits; ++i) {
        CHECK(getGlobalQubitIdx(sliced, i) == i);
    }

    auto even = QuregSlice(q, 0, 8, 2);
    CHECK(even.numQubits == 5);
    CHECK(even.start == 0);
    CHECK(even.end == 10);
    CHECK(getGlobalQubitIdx(even, 0) == 0);
    CHECK(getGlobalQubitIdx(even, 1) == 2);
    CHECK(getGlobalQubitIdx(even, 2) == 4);
    CHECK(getGlobalQubitIdx(even, 3) == 6);

    auto fewLast = QuregSlice(q, -5, -1, 1);
    CHECK(fewLast.numQubits == 5);
    CHECK(fewLast.start == -5);
    CHECK(fewLast.end == 0);
    for (auto i = 0; i < fewLast.numQubits; ++i) {
        CHECK(getGlobalQubitIdx(fewLast, i) == i + 7);
    }

    resetQascEnv();
}

TEST_CASE("classical declarations with QasmType.") {
    QasmType<int> v1(0);
    CHECK(v1 == 0);
    CHECK(v1.bits.size() == 32);
    CHECK(std::all_of(v1.begin(), v1.end(), [](auto v) { return v == 0; }));

    QasmType<int, 24> v2(0);
    CHECK(v2 == 0);
    CHECK(v2.bits.size() == 24);
    CHECK(std::all_of(v1.begin(), v1.end(), [](auto v) { return v == 0; }));

    QasmType<int, 24> v3(15);
    CHECK(v3 == 15);
    CHECK(v3.bits.size() == 24);
    // 15 == 0x11110...0
    for (auto i = 0; i < 4; ++i) {
        CHECK(v3[i] == 1);
    }
    for (auto i = 4; i < v3.bits.size(); ++i) {
        CHECK(v3[i] == 0);
    }

    QasmType<int, 24> v4(-15);
    CHECK(v4 == -15);
    CHECK(v4.bits.size() == 24);
    // -15 == 0x10001...1
    CHECK(v4[0] == 1);
    for (auto i = 1; i < 4; ++i) {
        CHECK(v4[i] == 0);
    }
    for (auto i = 4; i < v3.bits.size(); ++i) {
        CHECK(v4[i] == 1);
    }

    QasmType<unsigned int, 24> v5(15);
    CHECK(v5 == 15);
    CHECK(v5.bits.size() == 24);
    // 15 == 0x11110...0
    for (auto i = 0; i < 4; ++i) {
        CHECK(v5[i] == 1);
    }
    for (auto i = 4; i < v4.bits.size(); ++i) {
        CHECK(v5[i] == 0);
    }
    constexpr double eps = 1e-06;

    QasmType<unsigned int, 32, true> v6(0);
    CHECK(std::abs(v6 - 0.0) < eps);
    CHECK(v6.bits.size() == 32);
    CHECK(std::all_of(v6.begin(), v6.end(), [](auto v) { return v == 0; }));

    QasmType<unsigned int, 24, true> v7(0);
    CHECK(std::abs(v7 - 0.0) < eps);
    CHECK(v7.bits.size() == 24);
    CHECK(std::all_of(v7.begin(), v7.end(), [](auto v) { return v == 0; }));

    QasmType<unsigned int, 24, true> v8(0.32);
    CHECK(std::abs(v8 - 0.32) < eps);
    CHECK(v8.bits.size() == 24);
    // angle[24](0.32) == 000011010000100110111000
    // but our representation is in reversed order
    CHECK(v8[0] == 0);
    CHECK(v8[1] == 0);
    CHECK(v8[2] == 0);
    CHECK(v8[3] == 1);
    CHECK(v8[4] == 1);
    CHECK(v8[5] == 1);
    CHECK(v8[6] == 0);
    CHECK(v8[7] == 1);
    CHECK(v8[8] == 1);
    CHECK(v8[9] == 0);
    CHECK(v8[10] == 0);
    CHECK(v8[11] == 1);
    CHECK(v8[12] == 0);
    CHECK(v8[13] == 0);
    CHECK(v8[14] == 0);
    CHECK(v8[15] == 0);
    CHECK(v8[16] == 1);
    CHECK(v8[17] == 0);
    CHECK(v8[18] == 1);
    CHECK(v8[19] == 1);
    CHECK(v8[20] == 0);
    CHECK(v8[21] == 0);
    CHECK(v8[22] == 0);
    CHECK(v8[23] == 0);

    std::vector<QasmType<unsigned int, 32, true>> v9(2, 0);
    CHECK(std::all_of(v9.begin(), v9.end(),
                      [](auto v) { return v == 0 && v.bits.size() == 32; }));
    std::vector<QasmType<unsigned int, 24, true>> v10(2, 0);
    CHECK(std::all_of(v10.begin(), v10.end(),
                      [](auto v) { return v == 0 && v.bits.size() == 24; }));

    std::vector<QasmType<unsigned int, 24, true>> v11 = {0.32, 0.12};
    CHECK(v11[0].bits.size() == 24);

    CHECK(v11[0][0] == 0);
    CHECK(v11[0][1] == 0);
    CHECK(v11[0][2] == 0);
    CHECK(v11[0][3] == 1);
    CHECK(v11[0][4] == 1);
    CHECK(v11[0][5] == 1);
    CHECK(v11[0][6] == 0);
    CHECK(v11[0][7] == 1);
    CHECK(v11[0][8] == 1);
    CHECK(v11[0][9] == 0);
    CHECK(v11[0][10] == 0);
    CHECK(v11[0][11] == 1);
    CHECK(v11[0][12] == 0);
    CHECK(v11[0][13] == 0);
    CHECK(v11[0][14] == 0);
    CHECK(v11[0][15] == 0);
    CHECK(v11[0][16] == 1);
    CHECK(v11[0][17] == 0);
    CHECK(v11[0][18] == 1);
    CHECK(v11[0][19] == 1);
    CHECK(v11[0][20] == 0);
    CHECK(v11[0][21] == 0);
    CHECK(v11[0][22] == 0);
    CHECK(v11[0][23] == 0);

    // 0.12 == 000001001110001110100101
    CHECK(v11[1].bits.size() == 24);
    CHECK(v11[1][0] == 1);
    CHECK(v11[1][1] == 0);
    CHECK(v11[1][2] == 1);
    CHECK(v11[1][3] == 0);
    CHECK(v11[1][4] == 0);
    CHECK(v11[1][5] == 1);
    CHECK(v11[1][6] == 0);
    CHECK(v11[1][7] == 1);
    CHECK(v11[1][8] == 1);
    CHECK(v11[1][9] == 1);
    CHECK(v11[1][10] == 0);
    CHECK(v11[1][11] == 0);
    CHECK(v11[1][12] == 0);
    CHECK(v11[1][13] == 1);
    CHECK(v11[1][14] == 1);
    CHECK(v11[1][15] == 1);
    CHECK(v11[1][16] == 0);
    CHECK(v11[1][17] == 0);
    CHECK(v11[1][18] == 1);
    CHECK(v11[1][19] == 0);
    CHECK(v11[1][20] == 0);
    CHECK(v11[1][21] == 0);
    CHECK(v11[1][22] == 0);
    CHECK(v11[1][23] == 0);

    QasmType<unsigned int, 8> b1("0010_1010");
    CHECK(b1 == 42);
    QasmType<unsigned int, 8> b2(0b00101010);
    CHECK(b2 == 42);

    QasmType<unsigned int, 8> b3(rotl(b1, 3));
    CHECK(b3 == 81);

    QasmType<unsigned int, 8> b4(rotr(b1, 3));
    CHECK(b4 == 69);
}

namespace {
template <QasmQuregT T>
QasmType<unsigned int, 1> phaseMeasure(QasmType<unsigned int, 32, true> theta,
                                       T q) {
    rz(theta, q);
    h(q);
    return measure(q);
}

template <QasmQuregT T>
QasmType<unsigned int, 1> xCheck(T a, T b) {
    reset(b);
    for (auto i = 0; i < 3 + 1; i += 1)
        cx(a[i], b);
    return measure(b);
}

constexpr int parityTestN = 2;
QasmType<unsigned int, 1> parity(QasmType<unsigned int, parityTestN> aInput) {
    QasmType<unsigned int, 1> a(0);
    for (auto i = 0; i < parityTestN - 1 + 1; i += 1) {
        a ^= aInput[i];
    }
    return a;
}

template <typename T>
void onDetailedArray(const T&& arr) {
}

template <typename T>
void dynamicArray(T&& arr) {
}

template <typename T>
void mutableArrayRef(T&& arr) {
    arr[2] = 42;
}

template <typename VecT0, typename VecT1>
void mutableArrayRef(VecT0&& arr, VecT1&& b) {
    arr[2] = 42;
}

template <typename VecT0>
QasmType<int, 32> complexFuncOnArr(VecT0&& arr) {
    QasmType<unsigned int, 32> firstDim(qasmSizeof(arr, 0));
    QasmType<unsigned int, 32> sndDim(qasmSizeof(arr, 1));
    QasmType<int, 32> sum(0);
    for (auto i = 0; i < firstDim - 1 + 1; i += 1) {
        for (auto j = 0; j < sndDim - 1 + 1; j += 1) {
            sum += static_cast<QasmType<int, 32>>(arr[i][j]);
        }
    }
    return sum;
}

}    // namespace

TEST_CASE("testing generated functions definitions.") {
    SUBCASE("applying user defined phase measurement") {
        const auto numQubits = 1;
        auto q = QuregSlice(numQubits);
        initQascEnv(numQubits);

        h(q);
        auto result = phaseMeasure(std::numbers::pi, q);
        CHECK(result == 1);
        resetQascEnv();
    }

    SUBCASE("applying user defined x check") {
        SUBCASE("") {
            const auto numQubits = 5;
            auto a = QuregSlice(4);
            auto b = QuregSlice(1);
            initQascEnv(numQubits);
            auto result = xCheck(a, b);
            CHECK(result == 0);
            resetQascEnv();
        }

        SUBCASE("") {
            const auto numQubits = 5;
            auto a = QuregSlice(4);
            auto b = QuregSlice(1);
            initQascEnv(numQubits);
            x(a[0]);
            auto result = xCheck(a, b);
            CHECK(result == 1);
            resetQascEnv();
        }

        SUBCASE("") {
            const auto numQubits = 5;
            auto a = QuregSlice(4);
            auto b = QuregSlice(1);
            initQascEnv(numQubits);
            x(a[1]);
            auto result = xCheck(a, b);
            CHECK(result == 1);
            resetQascEnv();
        }

        SUBCASE("") {
            const auto numQubits = 5;
            auto a = QuregSlice(4);
            auto b = QuregSlice(1);
            initQascEnv(numQubits);
            x(a[2]);
            auto result = xCheck(a, b);
            CHECK(result == 1);
            resetQascEnv();
        }
    }

    SUBCASE("testing user defined parity check") {
        QasmType<unsigned int, 2> v1("00");
        QasmType<unsigned int, 2> v2("01");
        QasmType<unsigned int, 2> v3("10");
        QasmType<unsigned int, 2> v4("11");

        CHECK(parity(v1) == 0);
        CHECK(parity(v2) == 1);
        CHECK(parity(v3) == 1);
        CHECK(parity(v4) == 0);
    }

    SUBCASE("testing functions with arrays as parameters") {
        std::vector<QasmType<int, 16>> a(6, 0);
        std::vector<std::vector<QasmType<int, 16>>> b(
            5, std::vector<QasmType<int, 16>>(8, 0));
        dynamicArray(a);
        dynamicArray(getSlice(b[1], 0, 4, 1));
        mutableArrayRef(getSlice(a, 1, 3, 1));
        CHECK(a[3] == 42);
    }

    SUBCASE("testing complex function operating on array") {
        std::vector<std::vector<QasmType<int, 16>>> a = {{1, 2, 3, 4, 5},
                                                         {6, 7, 8, 9, 10}};
        QasmType<int, 32> res(complexFuncOnArr(a));
        CHECK(res == 55);
    }
}

TEST_CASE("testing assignments") {
    QasmType<int, 32> v0(10);
    QasmType<int, 32> v1(12);
    QasmType<unsigned int, 32, true> v2(0.15);
    QasmType<unsigned int, 1> v3(true);
    QasmType<unsigned int, 3> v4(0);
    double v5 = 5.5;

    CHECK(v0 + v1 == 22);
    if (v3) {
        auto v6 = v2 + v4;
        CHECK(v6 - 0.15 < testEps);
        auto v7 = v1 + v5;
        CHECK(v7 - 17.5 < testEps);
        v7 = v5 + v1;
        CHECK(v7 - 17.5 < testEps);
    }

    QasmType<unsigned int, 20> bitArr(0);
    for (auto& b : bitArr) {
        CHECK(b == 0);
    }

    QasmType<unsigned int, 8> bitArrFromStr("00001111");
    // Internally we have reversed ordering of bits
    for (size_t i = 0; i < 4; ++i) {
        CHECK(bitArrFromStr[i] == 1);
        CHECK(bitArrFromStr[i + 4] == 0);
    }

    QasmType<unsigned int, 32> u32_v(10);
    QasmType<int, 16> i16_v(0);
    i16_v = static_cast<QasmType<int, 16>>(u32_v);
    CHECK(i16_v == 10);

    double theta = 0.0;
    CHECK(static_cast<QasmType<unsigned int, 32, true>>(
              static_cast<QasmType<unsigned int, 32>>(
                  static_cast<QasmType<unsigned int, 32>>(theta))) ==
          static_cast<QasmType<unsigned int, 32, true>>(theta));
}

TEST_CASE("testing angle qasm type.") {
    QasmType<unsigned int, 4, true> pi1(std::numbers::pi);
    for (auto i = 0; i < 3; ++i) {
        CHECK(pi1[i] == 0);
    }
    CHECK(pi1[3] == 1);

    QasmType<unsigned int, 6, true> pi2(std::numbers::pi / 2.0);
    for (auto i = 0; i < 4; ++i) {
        CHECK(pi2[i] == 0);
    }
    CHECK(pi2[4] == 1);
    CHECK(pi2[5] == 0);

    QasmType<unsigned int, 8, true> pi3(7 * (std::numbers::pi / 8.0));

    for (auto i = 0; i < 4; ++i) {
        CHECK(pi3[i] == 0);
    }
    for (auto i = 4; i < 7; ++i) {
        CHECK(pi3[i] == 1);
    }
    CHECK(pi3[7] == 0);

    QasmType<unsigned int, 8, true> pi4((std::numbers::pi / 8.0));
    for (auto i = 0; i < 4; ++i) {
        CHECK(pi4[i] == 0);
    }
    CHECK(pi4[4] == 1);
    for (auto i = 5; i < 8; ++i) {
        CHECK(pi4[i] == 0);
    }
}

TEST_CASE("testing generated declaration and operations on complex type.") {
    std::complex<double> comp;
    comp = 2.5 + 3.5_im;
    std::complex<double> comp2 =
        2.0 * sin(3.14159265358979323846 / 2) + (3.1 * 5.5_im);
    double r = real(comp2);

    CHECK(r - 3.0 < testEps);
    CHECK(imag(comp) - 3.5 < testEps);
    CHECK(imag(comp2) - 17.05 < testEps);
}

TEST_CASE("testing compile time assignements.") {
    constexpr QasmType<unsigned int, 16> ui1 = 2 * 5;
    constexpr QasmType<unsigned int, 1> b1 = ui1[1];
    CHECK(b1 == 1);

    constexpr QasmType<unsigned int, 4> b2 = getSlice(ui1, 0, 3, 1);
    CHECK(b2[0] == 0);
    CHECK(b2[1] == 1);
    CHECK(b2[2] == 0);
    CHECK(b2[3] == 1);
}

TEST_CASE("testing generated code on arrays.") {
    std::vector<std::vector<QasmType<int, 32>>> a1 = {{0, 1, 2, 3, 4},
                                                      {5, 6, 7, 8, 9}};
    std::vector<std::vector<QasmType<int, 32>>> a2(
        10, std::vector<QasmType<int, 32>>(12, 0));

    QasmType<int, 32> qi1 = a1[0][1];
    CHECK(qi1 == 1);
    int i1 = a1[0][1];
    CHECK(i1 == 1);

    a2[3][2] = a1[0][1];
    CHECK(a2[3][2] == 1);

    auto a3 = getSlice(a1, 0, 1, 1);
    for (auto i = 0; i < a3.size(); ++i) {
        for (auto j = 0; j < a3[i].size(); ++j) {
            CHECK(a3[i][j] == a1[i][j]);
        }
    }

    auto a4 = getSlice(a2, 0, 5, 2);
    CHECK(a4.size() == 3);

    auto a5 = getSlice(a2, 8, 2, -2);
    CHECK(a5.size() == 4);

    auto a6 = getSlice(a2, -6, -1, 2);
    CHECK(a6.size() == 3);

    auto a7 = getSlice(a2, -7, -1, 2);
    CHECK(a7.size() == 4);

    auto a8 = getSlice(a2, -2, -6, -1);
    CHECK(a8.size() == 5);

    std::vector<QasmType<int, 32>> a9(5, 0);
    setSlice(a9, a1[0], 0, 4, 1);
    for (auto i = 0; i < a1[0].size(); ++i) {
        CHECK(a9[i] == a1[0][i]);
    }

    std::vector<QasmType<int, 32>> a10(5, 0);
    setSlice(a10, a1[0], 4, 0, -1);
    for (auto i = 0; i < a10.size(); ++i) {
        CHECK(a10[i] == a1[0][a10.size() - 1 - i]);
    }

    std::vector<QasmType<int, 32>> a11(5, 0);
    setSlice(a11, getSlice(a1[0], 2, 4, 1), 0, 2, 1);
    CHECK(a11[0] == a1[0][2]);
    CHECK(a11[1] == a1[0][3]);
    CHECK(a11[2] == a1[0][4]);
}

TEST_CASE("testing operations and slicing on bit type.") {
    QasmType<int, 32> i1("1111");
    CHECK(i1 == 15);

    QasmType<unsigned int, 1> lastBit = i1[0];
    CHECK(lastBit == 1);
    QasmType<unsigned int, 1> signBit = i1[31];
    CHECK(signBit == 0);
    QasmType<unsigned int, 1> alsoSignBit = i1[-1];
    CHECK(alsoSignBit == 0);
    QasmType<unsigned int, 16> even = getSlice(i1, 0, 31, 2);
    CHECK(even == 3);
    QasmType<unsigned int, 16> upperBits = getSlice(i1, -16, -1, 1);
    QasmType<unsigned int, 16> upperReversed = getSlice(i1, -1, -16, -1);

    setSlice(i1, "1010", 4, 7, 1);
    CHECK(i1 == 175);

    std::vector<QasmType<int, 32>> a1 = {0, 1, 2, 3, 4};
    a1[0][0] = 1;
    CHECK(a1[0] == 1);

    QasmType<unsigned int, 5> bits = getSlice(a1[4], 0, 4, 1);
    CHECK(bits[0] == 0);
    CHECK(bits[1] == 0);
    CHECK(bits[2] == 1);
    CHECK(bits[3] == 0);
}

TEST_CASE("testing operators on qasm types.") {
    QasmType<int, 16> i1(15);
    QasmType<int, 16> i2(12);
    QasmType<int, 16> i3 = i1 + i2;
    CHECK(i3 == 27);
    i3 = i1 + 3;
    CHECK(i3 == 18);
    i3 = 3 + i1;
    CHECK(i3 == 18);
    i3 = 3 + 3;
    CHECK(i3 == 6);
    i3 = 3 + 3.14 + i3;
    CHECK(i3 == 12);

    i3 = i1 - i2;
    CHECK(i3 == 3);
    i3 = i1 - 3;
    CHECK(i3 == 12);
    i3 = 3 - i1;
    CHECK(i3 == -12);
    i3 = 3 - 3;
    CHECK(i3 == 0);

    i3 = i1 * i2;
    CHECK(i3 == 180);
    i3 = i1 * 3;
    CHECK(i3 == 45);
    i3 = 3 * i1;
    CHECK(i3 == 45);
    i3 = 3 * 3;
    CHECK(i3 == 9);

    i3 = i1 / i2;
    CHECK(i3 == 1);
    i3 = i1 / 3;
    CHECK(i3 == 5);
    i3 = 3 / i1;
    CHECK(i3 == 0);
    i3 = 3 / 3;
    CHECK(i3 == 1);

    i3 = i1 & i2;
    CHECK(i3 == 12);
    i3 = i1 & 3;
    CHECK(i3 == 3);
    i3 = 3 & i1;
    CHECK(i3 == 3);
    i3 = 3 & 3;
    CHECK(i3 == 3);

    i3 = i1 | i2;
    CHECK(i3 == 15);
    i3 = i1 | 3;
    CHECK(i3 == 15);
    i3 = 3 | i1;
    CHECK(i3 == 15);
    i3 = 3 | 3;
    CHECK(i3 == 3);

    i3 = i1 ^ i2;
    CHECK(i3 == 3);
    i3 = i1 ^ 3;
    CHECK(i3 == 12);
    i3 = 3 ^ i1;
    CHECK(i3 == 12);
    i3 = 3 ^ 3;
    CHECK(i3 == 0);

    i3 = i1 << i2;
    CHECK(i3 == -4096);
    i3 = i1 << 3;
    CHECK(i3 == 120);
    i3 = 3 << i1;
    CHECK(i3 == -32768);
    i3 = 3 << 3;
    CHECK(i3 == 24);

    i3 = i1 >> i2;
    CHECK(i3 == 0);
    i3 = i1 >> 3;
    CHECK(i3 == 1);
    i3 = 3 >> i1;
    CHECK(i3 == 0);
    i3 = 3 >> 3;
    CHECK(i3 == 0);

    CHECK(~i1 == -16);
    i1 = 15;
    CHECK(-i1 == -15);
    i1 = 15;

    bool logicalTest = false;
    logicalTest = i1 >= i2;
    CHECK(logicalTest == true);
    logicalTest = i1 > i2;
    CHECK(logicalTest == true);
    logicalTest = i1 < i2;
    CHECK(logicalTest == false);

    logicalTest = i1 <= i2;
    CHECK(logicalTest == false);
    logicalTest = i1 == i2;
    CHECK(logicalTest == false);
    logicalTest = i1 != i2;
    CHECK(logicalTest == true);
    logicalTest = i1 && i2;
    CHECK(logicalTest == true);
    logicalTest = i1 || i2;
    CHECK(logicalTest == true);

    int someInt = 2;
    logicalTest = i1 <= 0;
    CHECK(logicalTest == false);
    logicalTest = i1 == 0;
    CHECK(logicalTest == false);
    logicalTest = i1 != 0;
    CHECK(logicalTest == true);
    logicalTest = i1 && someInt;
    CHECK(logicalTest == true);
    logicalTest = i1 || someInt;
    CHECK(logicalTest == true);

    logicalTest = 1 <= i2;
    CHECK(logicalTest == true);
    logicalTest = 1 == i2;
    CHECK(logicalTest == false);
    logicalTest = 1 != i2;
    CHECK(logicalTest == true);
    logicalTest = 1 && i2;
    CHECK(logicalTest == true);
    logicalTest = 1 || i2;
    CHECK(logicalTest == true);

    auto pop = popcount(i1);
    CHECK(pop == 4);
}
