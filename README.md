# interpolator_c-
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include "FieldParser.h"
#include "TrilinearInterpolator.h"
#include "TricubicInterpolator.h"

void test_point(std::ofstream& outFile, TrilinearInterpolator& trilinear, TricubicInterpolator& tricubic, const Vector3D& pt)
{
    Vector3D B_lin = trilinear.interpolate(pt);
    Vector3D B_cub = tricubic.interpolate(pt);
            

    double err_x = B_lin[0] - B_cub[0];
    double err_y = B_lin[1] - B_cub[1];
    double err_z = B_lin[2] - B_cub[2];

    outFile << std::scientific << std::setprecision(5) 
        << pt[0] << "\t" << pt[1] << "\t" << pt[2] << "\t"
        << B_lin[0] << "\t" << B_lin[1] << "\t" << B_lin[2] << "\t"
        << B_cub[0] << "\t" << B_cub[1] << "\t" << B_cub[2] << "\t"
        << err_x << "\t" << err_y << "\t" << err_z << "\n";
}

// // === Аналитическая модель ===
// Vector3D analyticalField(const Vector3D& pos) {
//     constexpr double mu0_over_4pi = 1e-7;
//     Vector3D m = {0.0, 0.0, 1e-6}; // диполь вдоль Z
//     double x = pos[0], y = pos[1], z = pos[2];
//     double r2 = x*x + y*y + z*z;
//     if (r2 < 1e-16) return {0, 0, 0};
//     double r5 = r2 * r2 * std::sqrt(r2);
//     double dot = m[0]*x + m[1]*y + m[2]*z;
//     return {
//         mu0_over_4pi * (3*x*dot - m[0]*r2) / r5,
//         mu0_over_4pi * (3*y*dot - m[1]*r2) / r5,
//         mu0_over_4pi * (3*z*dot - m[2]*r2) / r5
//     };
// }

int main(int argc, const char **argv) {

    if (argc < 3) {
        std::cerr << "insufficient parametrs in comand line\n";
        std::cerr << "Usage: " << argv[0] << " field_data_file output_file\n";
        exit(1);
    }
    //std::cout << "Введите путь к файлу с данными (например: field_data.txt): ";
    std::string inputFilename = argv[1];
    //std::getline(std::cin, inputFilename);

    // Удаление \r для Windows
    //if (!inputFilename.empty() && inputFilename.back() == '\r') {
    //    inputFilename.pop_back();
    //}

    const std::string outputFilename = argv[2]; //"interpolation_results.txt";
    const char separator = '\t'; // или ' '
    const double step = 0.02;

    try {
        // Проверка существования файла
        std::ifstream testFile(inputFilename);
        if (!testFile.is_open()) {
            std::cerr << "Ошибка: файл '" << inputFilename << "' не найден.\n";
            return 1;
        }
        testFile.close();

        // Сканирование границ
        auto bounds = FieldParser::scanBounds(inputFilename, separator);
        std::cout << "Границы:\n";
        std::cout << "  X: [" << bounds.xMin << ", " << bounds.xMax << "]\n";
        std::cout << "  Y: [" << bounds.yMin << ", " << bounds.yMax << "]\n";
        std::cout << "  Z: [" << bounds.zMin << ", " << bounds.zMax << "]\n";

        // вычисление размеров сетки
        // const int nx = 142, ny = 142, nz = 135;
        // const size_t expectedPoints = static_cast<size_t>(nx) * ny * nz;
        int nx = static_cast<int>(std::round((bounds.xMax - bounds.xMin) / step)) + 1;
        int ny = static_cast<int>(std::round((bounds.yMax - bounds.yMin) / step)) + 1;
        int nz = static_cast<int>(std::round((bounds.zMax - bounds.zMin) / step)) + 1;
        std::cout << "Вычисленные размеры сетки:\n";
        std::cout << "  nx = " << nx << ", ny = " << ny << ", nz = " << nz << "\n";
        const size_t expectedPoints = static_cast<size_t>(nx) * ny * nz;
        std::cout << "Ожидаемое число точек: " << expectedPoints << "\n";

        // Парсинг данных
        auto nodes = FieldParser::parseIrregular(inputFilename, separator);
        std::cout << "Прочитано точек: " << nodes.size() << "\n";

        if (nodes.size() != expectedPoints) {
            std::cerr << "Предупреждение: число точек не совпадает с ожидаемым (" 
                      << expectedPoints << ").\n";
            std::cerr << "Убедитесь, что данные лежат на регулярной сетке " 
                      << nx << "x" << ny << "x" << nz << ".\n";
        }

        // Подготовка значений поля
        std::vector<Vector3D> fieldValues;
        fieldValues.reserve(nodes.size());
        for (const auto& node : nodes) {
            fieldValues.push_back(node.field);
        }

        std::cout << "Создание интерполяторов (trilinear)\n";

        TrilinearInterpolator trilinear(
            bounds.xMin, bounds.xMax, nx,
            bounds.yMin, bounds.yMax, ny,
            bounds.zMin, bounds.zMax, nz,
            fieldValues
        );


        std::cout << "Создание интерполяторов (tricubic)\n";
        TricubicInterpolator tricubic(
            bounds.xMin, bounds.xMax, nx,
            bounds.yMin, bounds.yMax, ny,
            bounds.zMin, bounds.zMax, nz,
            fieldValues
        );

        std::cout << "Тестирование и запись результатов" << std::endl;
        std::ofstream outFile(outputFilename);
        if (!outFile.is_open()) {
            throw std::runtime_error("Не удалось создать выходной файл");
        }

        outFile << "# X\tY\tZ\t"
                << "Bx_lin\tBy_lin\tBz_lin\t"
                << "Bx_cub\tBy_cub\tBz_cub\t"
                //<< "Bx_exact\tBy_exact\tBz_exact\t"
                //<< "Error_lin\tError_cub\n";
                << "err_x\terr_y\terr_z\n";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> disX(bounds.xMin, bounds.xMax);
        std::uniform_real_distribution<double> disY(bounds.yMin, bounds.yMax);
        std::uniform_real_distribution<double> disZ(bounds.zMin, bounds.zMax);

        for (int i = 0; i < 8; ++i)
        {
            testPoints.push_back({disX(gen), disY(gen), disZ(gen)});
        }


        test_point(outFile, trilinear, tricubic, {0.0, 0.0, 0.0});
        test_point(outFile, trilinear, tricubic, {1.41, 1.41, 1.34});
        test_point(outFile, trilinear, tricubic, {1.42, 2.16, 2.08}); 
        outFile << "# 1375754	1.420000000000E+000	2.160000000000E+000	2.080000000000E+000	-1.199149342689E-003	6.911488203521E-003	-2.605997325457E-001	2.606941258088E-001\n";



        const size_t numTests = 10;
        for (size_t i = 0; i < numTests; ++i) {
            Vector3D pt = {disX(gen), disY(gen), disZ(gen)};
            test_point(outFile, trilinear, tricubic, pt);
        }

        outFile.close();
        std::cout << "Результаты записаны в " << outputFilename << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
