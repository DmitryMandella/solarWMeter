idf_component_register(
    SRCS "main.c" "sim800.cpp"   # ← здесь важный момент
    INCLUDE_DIRS "."             # путь к sim800.h
)