##-*****************************************************************************
##
## Copyright (c) 2009-2015,
##  Sony Pictures Imageworks Inc. and
##  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
##
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
## *       Redistributions of source code must retain the above copyright
## notice, this list of conditions and the following disclaimer.
## *       Redistributions in binary form must reproduce the above
## copyright notice, this list of conditions and the following disclaimer
## in the documentation and/or other materials provided with the
## distribution.
## *       Neither the name of Industrial Light & Magic nor the names of
## its contributors may be used to endorse or promote products derived
## from this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
##-*****************************************************************************

SET(TEST_LIBS
    AbcWFObjConvert
    Alembic::Alembic
    ${HDF5_LIBRARIES}
    ${ALEMBIC_ILMBASE_LIBS}
    ${ZSTD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARIES} 
    ${EXTERNAL_MATH_LIBS}
)

#ADD_EXECUTABLE(WFObjConvert_ParserTest parserTest.cpp)
ADD_EXECUTABLE(WFObjConvert_ParserTest test1.cpp)
TARGET_LINK_LIBRARIES(WFObjConvert_ParserTest ${TEST_LIBS})

ADD_EXECUTABLE(WFObjConvert_obj2abc test2.cpp)
TARGET_LINK_LIBRARIES( WFObjConvert_obj2abc ${TEST_LIBS})

#ADD_TEST(AbcClients_WFObjConvert_Parser_TEST WFObjConvert_ParserTest)
#ADD_TEST(AbcClients_WFObjConvert_obj2abc_TEST WFObjConvert_obj2abc)
