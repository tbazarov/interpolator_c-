#pragma once
#include "MagneticFieldInterpolator.h"
#include <vector>
#include <array>
#include <iostream>

/// Трикубический интерполятор для регулярной 3D-сетки
class TricubicInterpolator : public MagneticFieldInterpolator {
private:
    // Границы области
    double m_xMin, m_xMax;
    double m_yMin, m_yMax;
    double m_zMin, m_zMax;

    // Размеры сетки
    int m_nx, m_ny, m_nz;

    // Шаги сетки
    double m_dx, m_dy, m_dz;

    // Коэффициенты трикубического полинома: [ячейка][компонент][64]
    std::vector<std::array<std::array<double, 64>, 3>> m_coeffs;

    // Статическая матрица коэффициентов
    static std::array<std::array<int, 64>, 64> s_coeffMatrix;
    static bool s_matrixLoaded;

    // Вспомогательные методы
    bool insideBounds(const Vector3D& pos) const;
    size_t cellIndex(int i, int j, int k) const;
    size_t nodeIndex(int i, int j, int k) const;
    double evaluatePolynomial(const std::array<double, 64>& coeffs, double tx, double ty, double tz) const;

    // Численные производные и коэффициенты
    void computeAllDerivatives(
        const std::vector<double>& f,
        int i, int j, int k,
        double& f_val,
        double& fx, double& fy, double& fz,
        double& fxy, double& fyz, double& fzx,
        double& fxyz
    ) const;

    void computeCoefficientsFromDerivatives(
        const std::array<double, 8>& f,
        const std::array<double, 8>& fx,
        const std::array<double, 8>& fy,
        const std::array<double, 8>& fz,
        const std::array<double, 8>& fxy,
        const std::array<double, 8>& fyz,
        const std::array<double, 8>& fzx,
        const std::array<double, 8>& fxyz,
        std::array<double, 64>& coeffs
    );

public:
    TricubicInterpolator(
        double xMin, double xMax, int nx,
        double yMin, double yMax, int ny,
        double zMin, double zMax, int nz,
        const std::vector<Vector3D>& fieldValues
    );

    Vector3D interpolate(const Vector3D& position) const override;
};
