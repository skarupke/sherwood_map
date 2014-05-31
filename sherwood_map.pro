TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS += -Wextra
release: QMAKE_CXXFLAGS += -g
address_sanitizer: QMAKE_CXXFLAGS += -fsanitize=address
address_sanitizer: QMAKE_CXXFLAGS += -fsanitize=use-after-return
address_sanitizer: QMAKE_LFLAGS += -fsanitize=address
static_analysis: QMAKE_CXXFLAGS -= -Qunused-arguments

LIBS += /usr/lib/libgtest.a
LIBS += -pthread

SOURCES += main.cpp \
    boost_test/exception/assign_exception_tests.cpp \
    boost_test/exception/constructor_exception_tests.cpp \
    boost_test/exception/copy_exception_tests.cpp \
    boost_test/exception/erase_exception_tests.cpp \
    boost_test/exception/insert_exception_tests.cpp \
    boost_test/exception/rehash_exception_tests.cpp \
    boost_test/exception/swap_exception_tests.cpp \
    boost_test/unordered/allocator_traits.cpp \
    boost_test/unordered/assign_tests.cpp \
    boost_test/unordered/at_tests.cpp \
    boost_test/unordered/bucket_tests.cpp \
    boost_test/unordered/compile_map.cpp \
    boost_test/unordered/compile_set.cpp \
    boost_test/unordered/constructor_tests.cpp \
    boost_test/unordered/copy_tests.cpp \
    boost_test/unordered/equality_tests.cpp \
    boost_test/unordered/equivalent_keys_tests.cpp \
    boost_test/unordered/erase_equiv_tests.cpp \
    boost_test/unordered/erase_tests.cpp \
    boost_test/unordered/find_tests.cpp \
    boost_test/unordered/fwd_map_test.cpp \
    boost_test/unordered/fwd_set_test.cpp \
    boost_test/unordered/incomplete_test.cpp \
    boost_test/unordered/insert_stable_tests.cpp \
    boost_test/unordered/insert_tests.cpp \
    boost_test/unordered/link_test_1.cpp \
    boost_test/unordered/link_test_2.cpp \
    boost_test/unordered/load_factor_tests.cpp \
    boost_test/unordered/minimal_allocator.cpp \
    boost_test/unordered/move_tests.cpp \
    boost_test/unordered/noexcept_tests.cpp \
    boost_test/unordered/rehash_tests.cpp \
    boost_test/unordered/simple_tests.cpp \
    boost_test/unordered/swap_tests.cpp \
    boost_test/unordered/unnecessary_copy_tests.cpp \
    finished/sherwood_map.cpp \
    sherwood_map_test.cpp \
    profiling/unique_running_insertion.cpp \
    profiling/unique_scattered_erasure.cpp \
    profiling/unique_scattered_lookup.cpp

HEADERS += \
    boost_test/exception/containers.hpp \
    boost_test/helpers/check_return_type.hpp \
    boost_test/helpers/count.hpp \
    boost_test/helpers/equivalent.hpp \
    boost_test/helpers/exception_test.hpp \
    boost_test/helpers/fwd.hpp \
    boost_test/helpers/generators.hpp \
    boost_test/helpers/helpers.hpp \
    boost_test/helpers/input_iterator.hpp \
    boost_test/helpers/invariants.hpp \
    boost_test/helpers/list.hpp \
    boost_test/helpers/memory.hpp \
    boost_test/helpers/metafunctions.hpp \
    boost_test/helpers/postfix.hpp \
    boost_test/helpers/prefix.hpp \
    boost_test/helpers/random_values.hpp \
    boost_test/helpers/strong.hpp \
    boost_test/helpers/test.hpp \
    boost_test/helpers/tracker.hpp \
    boost_test/objects/cxx11_allocator.hpp \
    boost_test/objects/exception.hpp \
    boost_test/objects/fwd.hpp \
    boost_test/objects/minimal.hpp \
    boost_test/objects/test.hpp \
    boost_test/unordered/compile_tests.hpp \
    sherwood_map.hpp \
    finished/sherwood_map.hpp \
    sherwood_test.hpp \
    profiling/profile_shared.h \
    sherwood_map_two_array.hpp

OTHER_FILES +=

