#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include "FieldParser.h"
#include "TrilinearInterpolator.h"
#include "TricubicInterpolator.h"

void test_point(std::ofstream& outFile, 
                const TrilinearInterpolator& trilinear, 
                const TricubicInterpolator& tricubic, 
                const Vector3D& pt)
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

int main(int argc, const char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_field_file> <output_results_file>\n";
        return 1;
    }

    std::string inputFilename = argv[1];
    std::string outputFilename = argv[2];
    const char separator = '\t';
    const double step = 0.02;

    try {
        // Проверка существования файла
        std::ifstream testFile(inputFilename);
        if (!testFile.is_open()) {
            std::cerr << "Error: cannot open input file '" << inputFilename << "'\n";
            return 1;
        }
        testFile.close();

        // Сканирование границ
        auto bounds = FieldParser::scanBounds(inputFilename, separator);
        std::cout << "Bounds:\n";
        std::cout << "  X: [" << bounds.xMin << ", " << bounds.xMax << "]\n";
        std::cout << "  Y: [" << bounds.yMin << ", " << bounds.yMax << "]\n";
        std::cout << "  Z: [" << bounds.zMin << ", " << bounds.zMax << "]\n";

        // Вычисление размеров сетки
        int nx = static_cast<int>(std::round((bounds.xMax - bounds.xMin) / step)) + 1;
        int ny = static_cast<int>(std::round((bounds.yMax - bounds.yMin) / step)) + 1;
        int nz = static_cast<int>(std::round((bounds.zMax - bounds.zMin) / step)) + 1;
        size_t expectedPoints = static_cast<size_t>(nx) * ny * nz;

        std::cout << "Grid size: " << nx << "x" << ny << "x" << nz << " (" << expectedPoints << " points)\n";

        // Парсинг данных
        auto nodes = FieldParser::parseIrregular(inputFilename, separator);
        std::cout << "Read " << nodes.size() << " points\n";

        if (nodes.size() != expectedPoints) {
            std::cerr << "Warning: point count mismatch! Expected " << expectedPoints << "\n";
        }

        // Подготовка значений поля
        std::vector<Vector3D> fieldValues;
        fieldValues.reserve(nodes.size());
        for (const auto& node : nodes) {
            fieldValues.push_back(node.field);
        }

        // Создание интерполяторов
        std::cout << "Creating interpolators...\n";
        TrilinearInterpolator trilinear(
            bounds.xMin, bounds.xMax, nx,
            bounds.yMin, bounds.yMax, ny,
            bounds.zMin, bounds.zMax, nz,
            fieldValues
        );

        TricubicInterpolator tricubic(
            bounds.xMin, bounds.xMax, nx,
            bounds.yMin, bounds.yMax, ny,
            bounds.zMin, bounds.zMax, nz,
            fieldValues
        );

        // Генерация тестовых точек
        std::vector<Vector3D> testPoints;

        // 8 вершин куба
        double corners[8][3] = {
            {bounds.xMin, bounds.yMin, bounds.zMin},
            {bounds.xMax, bounds.yMin, bounds.zMin},
            {bounds.xMin, bounds.yMax, bounds.zMin},
            {bounds.xMax, bounds.yMax, bounds.zMin},
            {bounds.xMin, bounds.yMin, bounds.zMax},
            {bounds.xMax, bounds.yMin, bounds.zMax},
            {bounds.xMin, bounds.yMax, bounds.zMax},
            {bounds.xMax, bounds.yMax, bounds.zMax}
        };
        for (int i = 0; i < 8; ++i) {
            testPoints.push_back({corners[i][0], corners[i][1], corners[i][2]});
        }

        // Центр куба
        testPoints.push_back({
            (bounds.xMin + bounds.xMax) / 2.0,
            (bounds.yMin + bounds.yMax) / 2.0,
            (bounds.zMin + bounds.zMax) / 2.0
        });

        // 8 случайных точек
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> disX(bounds.xMin, bounds.xMax);
        std::uniform_real_distribution<double> disY(bounds.yMin, bounds.yMax);
        std::uniform_real_distribution<double> disZ(bounds.zMin, bounds.zMax);

        for (int i = 0; i < 8; ++i) {
            testPoints.push_back({disX(gen), disY(gen), disZ(gen)});
        }

        // Тестирование
        std::cout << "Testing " << testPoints.size() << " points...\n";
        std::ofstream outFile(outputFilename);
        if (!outFile.is_open()) {
            throw std::runtime_error("Cannot create output file");
        }

        outFile << "# X\tY\tZ\t"
                << "Bx_lin\tBy_lin\tBz_lin\t"
                << "Bx_cub\tBy_cub\tBz_cub\t"
                << "err_x\terr_y\terr_z\n";

        for (const auto& pt : testPoints) {
            test_point(outFile, trilinear, tricubic, pt);
        }

        outFile.close();
        std::cout << "Results written to " << outputFilename << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}