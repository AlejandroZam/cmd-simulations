#pragma once
#include <Eigen/Dense>
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

// Read an Eigen column vector from a YAML sequence.
// YAML:  vec: [1.0, 2.0, 3.0]
// Usage: auto v = yamlToVector<Eigen::Vector3d>(node["vec"]);
template <typename Vec>
Vec yamlToVector(const YAML::Node& node) {
    if (!node || !node.IsSequence())
        throw std::runtime_error("yaml_eigen: expected a YAML sequence for vector");
    Vec v(static_cast<Eigen::Index>(node.size()));
    for (std::size_t i = 0; i < node.size(); ++i)
        v(static_cast<Eigen::Index>(i)) = node[i].as<double>();
    return v;
}

// Read an Eigen dynamic vector from a YAML sequence (size inferred).
// Usage: auto v = yamlToVectorXd(node["vec"]);
inline Eigen::VectorXd yamlToVectorXd(const YAML::Node& node) {
    return yamlToVector<Eigen::VectorXd>(node);
}

// Read an Eigen matrix from a YAML sequence-of-sequences (row-major).
// YAML:
//   mat:
//     - [1.0, 0.0]
//     - [0.0, 1.0]
// Usage: auto m = yamlToMatrixXd(node["mat"]);
inline Eigen::MatrixXd yamlToMatrixXd(const YAML::Node& node) {
    if (!node || !node.IsSequence())
        throw std::runtime_error("yaml_eigen: expected a YAML sequence for matrix");
    int rows = static_cast<int>(node.size());
    if (rows == 0) return Eigen::MatrixXd(0, 0);
    int cols = static_cast<int>(node[0].size());
    Eigen::MatrixXd m(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            m(r, c) = node[r][c].as<double>();
    return m;
}

// Write an Eigen vector to a YAML sequence node.
inline YAML::Node eigenToYaml(const Eigen::VectorXd& v) {
    YAML::Node node(YAML::NodeType::Sequence);
    for (Eigen::Index i = 0; i < v.size(); ++i)
        node.push_back(v(i));
    return node;
}
