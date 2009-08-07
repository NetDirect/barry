SET(TARGET_SRC
	jdwp.cc
	data.cc
	message.cc
	handler.cc
	server.cc
	manager.cc
)

INCLUDE_DIRECTORIES(
	${bjdwp_SOURCE_DIR}/src
	${bjdwp_SOURCE_DIR}/include/barry/jdwp
)

INCLUDE_DIRECTORIES(${BARRY_INCLUDE_DIRS})
LINK_DIRECTORIES(${BARRY_LIBRARY_DIRS})

ADD_LIBRARY(${LIB_JDWP_NAME} SHARED ${TARGET_SRC})

