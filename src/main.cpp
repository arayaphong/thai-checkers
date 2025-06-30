#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <ranges>

// C++20 features demonstration

int main() {
    std::cout << "Hello, Thai Checkers 2!" << std::endl;
    std::cout << "This project uses C++20 features." << std::endl;
    
    // C++20 feature: auto type deduction improvements
    auto numbers = std::vector{1, 2, 3, 4, 5};
    
    // C++20 feature: ranges
    std::cout << "Numbers: ";
    for (const auto& num : numbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    
    // C++20 feature: string formatting (if available)
    std::string message = "Project initialized successfully!";
    std::cout << message << std::endl;
    
    return 0;
}
