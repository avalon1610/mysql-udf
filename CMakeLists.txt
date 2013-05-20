PROJECT(udf_example)

# Path for MySQL include directory
INCLUDE_DIRECTORIES("D:/mysql-server/mysql-5.1/include")

ADD_DEFINITIONS("-DHAVE_DLOPEN")
ADD_LIBRARY(udf_example MODULE udf_example.c udf_example.def)
TARGET_LINK_LIBRARIES(udf_example wsock32)
