#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

inline bool load_csv(const std::string& path, std::vector<double>& M, int& n) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::vector<std::vector<double>> rows;
    std::string line;
    while (std::getline(file, line)) {
        while (!line.empty() && (line.back() == ',' || line.back() == ' ' || line.back() == '\r')) {
            line.pop_back();
        }
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::vector<double> row;
        double val;
        
        while (ss >> val) {
            row.push_back(val);
            int peeked = ss.peek();
            if (peeked == ',' || peeked == ' ') {
                ss.ignore();
            }
        }
        
        if (!row.empty()) rows.push_back(std::move(row));
    }
    
    if (rows.empty()) return false;
    
    n = static_cast<int>(rows.size());
    M.resize(n * n);
    for (int i = 0; i < n; ++i) {
        if (static_cast<int>(rows[i].size()) != n) return false;
        for (int j = 0; j < n; ++j) {
            M[i * n + j] = rows[i][j];
        }
    }
    return true;
}
