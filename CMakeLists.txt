cmake_minimum_required(VERSION 3.29)
project(projetSystemeComplet C)

set(CMAKE_C_STANDARD 23)

add_executable(projetSystemeComplet main.c
        serveur.c
        client.c
        test.c
        server2.c)
