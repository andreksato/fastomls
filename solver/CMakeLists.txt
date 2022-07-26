cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}) 

project(fastomls_solver)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(../common)

find_package(Qt5Widgets)
find_package(Qt5Gui)
find_package(Qt5Svg)

set(CMAKE_AUTOMOC ON)

set(SRC
	mainConsole.cpp
	consolepackingloader.cpp
	packingParametersParser.cpp
	../common/packingproblem.cpp
	raster/totaloverlapmap.cpp
	raster/rasterstrippackingsolver.cpp
	raster/rasterstrippackingsolver2d.cpp
	raster/rasteroverlapevaluator.cpp
	raster/rasteroverlapevaluatorgls.cpp
	raster/rasteroverlapevaluatordoublegls.cpp
	raster/rasterpackingsolution.cpp
	raster/rasterpackingproblem.cpp
	raster/rasternofitpolygon.cpp
	raster/rasterinnerfitpolygon.cpp
	raster/packingthread.cpp
	raster/packing2dthread.cpp
	raster/packingclusterthread.cpp
	raster/glsweightset.cpp
	)
	
set(HEADERS
	consolepackingloader.h
	packingParametersParser.h
	../common/packingproblem.h
	raster/totaloverlapmap.h
	raster/rasterstrippackingsolver.h
	raster/rasteroverlapevaluator.h
	raster/rasterpackingsolution.h
	raster/rasterstrippackingparameters.h
	raster/rasterpackingproblem.h
	raster/rasternofitpolygon.h
	raster/rasterinnerfitpolygon.h
	raster/packingthread.h
	raster/packing2dthread.h
	raster/packingclusterthread.h
	raster/glsweightset.h
	)

add_executable(fastomls_solver ${SRC} ${HEADERS})

target_link_libraries(fastomls_solver Qt5::Core Qt5::Gui Qt5::Svg)

set_target_properties(fastomls_solver PROPERTIES COMPILE_DEFINITIONS "CONSOLE")
