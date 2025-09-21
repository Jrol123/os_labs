# Указываем минимальную требуемую версию CMake
cmake_minimum_required(VERSION 4.1)

# Задаем имя проекта и язык (C++)
project(LAB_1 CXX)

# Устанавливаем стандарт языка C++ (например, C++17)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка для компиляции в режиме отладки (по умолчанию)
# Это включает отладочную информацию для gdb
set(CMAKE_BUILD_TYPE Debug)

# Сообщаем CMake, что мы хотим создать исполняемый файл
# из исходного кода, находящегося в файле src/main.cpp
add_executable(LAB_1 src/main.cpp)

# Если бы у нас были заголовки в папке include, мы бы сообщили компилятору, где их искать
# target_include_directories(HelloWorld PRIVATE include)