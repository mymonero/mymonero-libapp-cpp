
cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(TEST)
enable_testing()

#Find openSSL
find_package(OpenSSL REQUIRED)

#Prep ourselves for compiling boost
find_package(Boost COMPONENTS 
    unit_test_framework REQUIRED
    atomic REQUIRED
    chrono REQUIRED
    context REQUIRED
    date_time REQUIRED
    exception REQUIRED
    filesystem REQUIRED
    graph REQUIRED
    iostreams REQUIRED
    math_c99 REQUIRED
    math_c99f REQUIRED
    math_c99l REQUIRED
    math_tr1 REQUIRED
    math_tr1f REQUIRED
    math_tr1l REQUIRED
    prg_exec_monitor REQUIRED
    program_options REQUIRED
    random REQUIRED
    regex REQUIRED
    serialization REQUIRED
    system REQUIRED
    test_exec_monitor REQUIRED
    thread REQUIRED
    timer REQUIRED
    wave REQUIRED
    wserialization REQUIRED
)

include_directories(${Boost_INCLUDE_DIRS})
include_directories("src")
include_directories("test")
include_directories("src/contrib")

set(MYMONERO_CORE_CPP "src/mymonero-core-cpp")
set(MYMONERO_CORE_CPP_SRC "${MYMONERO_CORE_CPP}/src")
include_directories("${MYMONERO_CORE_CPP}/src")
#
set(MONERO_SRC "${MYMONERO_CORE_CPP}/contrib/monero-core-custom")
include_directories(${MONERO_SRC})
include_directories("${MONERO_SRC}/epee/include")
include_directories("${MONERO_SRC}/common")
include_directories("${MONERO_SRC}/vtlogger")
include_directories("${MONERO_SRC}/crypto")
include_directories("${MONERO_SRC}/cryptonote_basic")
include_directories("${MONERO_SRC}/multisig")
include_directories("${MONERO_SRC}/cryptonote_core")
include_directories("${MONERO_SRC}/cryptonote_protocol")
include_directories("${MONERO_SRC}/wallet")
include_directories("${MONERO_SRC}/rpc")
include_directories("${MONERO_SRC}/mnemonics")
include_directories("${MONERO_SRC}/contrib/libsodium/include") # support sodium/… paths
include_directories("${MONERO_SRC}/contrib/libsodium/include/sodium")


# keeping test files in a separate source directory 
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/test_*.cpp)

set(
    SRC_FILES
    #
    test/tests_common.hpp
    #
	src/Dispatch/Dispatch_Interface.hpp
	src/Dispatch/Dispatch.asio.hpp
	src/UserIdle/UserIdle.hpp
	src/UserIdle/UserIdle.cpp
    src/Passwords/PasswordController.hpp
    src/Passwords/PasswordController.cpp
	src/Settings/SettingsProviders.hpp
	src/Settings/SettingsProviders.cpp
    src/Settings/SettingsController.hpp
    src/Settings/SettingsController.cpp
	src/Lists/PersistedObjectListController.hpp
	src/Lists/PersistedObjectListController.cpp
	src/Wallets/Wallet.hpp
	src/Wallets/Wallet.cpp
	src/Wallets/WalletsListController_Base.hpp
	src/Wallets/WalletsListController_Base.cpp
	src/Wallets/WalletsListController.Full.hpp
	src/Wallets/WalletsListController.Full.cpp
	src/Wallets/WalletsListController.Lite.hpp
	src/Wallets/WalletsListController.Lite.cpp
	src/Wallets/Wallet_HostPollingController.hpp
	src/Wallets/Wallet_HostPollingController.cpp
	src/Wallets/Wallet_TxCleanupController.hpp
	src/Wallets/Wallet_TxCleanupController.cpp
	src/Wallets/Wallet_KeyImageCache.hpp
	src/Wallets/Wallet_KeyImageCache.cpp
    src/APIClient/root_certificates.hpp
    src/Currencies/Currencies.hpp
    src/Currencies/Currencies.cpp
    src/Persistence/document_persister.hpp
    src/Persistence/document_persister.cpp
    src/Persistence/PersistableObject.hpp
    src/Persistence/PersistableObject.cpp
    src/AppBridge/AppBridge.hpp
    src/AppBridge/AppBridge.cpp
    src/AppBridge/HTTPRequests_Handle_bridge.hpp
    src/App/AppServiceLocator.hpp
    src/App/AppServiceLocator.asio.cpp
    src/SendFunds/Controllers/SendFundsFormSubmissionController.hpp
    src/SendFunds/Controllers/SendFundsFormSubmissionController.cpp
    src/APIClient/HTTPRequests_Interface.hpp
    src/APIClient/HTTPRequests.beast.hpp
    src/APIClient/HostedMonero.cpp
    src/APIClient/HostedMonero.hpp
	src/APIClient/parsing.hpp
	src/OpenAlias/OpenAlias.hpp
	src/OpenAlias/OpenAlias.cpp
    src/RNCryptor-C/rncryptor_c.h
    src/RNCryptor-C/rncryptor_c.c
    src/RNCryptor-C/mutils.h
    src/RNCryptor-C/mutils.c
    src/base64/base64.hpp
    #
    src/contrib/rapidjson_defines.hpp
	#
	${MYMONERO_CORE_CPP_SRC}/monero_address_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_address_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_paymentID_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_paymentID_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_key_image_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_key_image_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_fee_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_fee_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_transfer_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_transfer_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_fork_rules.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_fork_rules.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_wallet_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_wallet_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/serial_bridge_index.hpp
	${MYMONERO_CORE_CPP_SRC}/serial_bridge_index.cpp
	${MYMONERO_CORE_CPP_SRC}/monero_send_routine.hpp
	${MYMONERO_CORE_CPP_SRC}/monero_send_routine.cpp
	${MYMONERO_CORE_CPP_SRC}/serial_bridge_utils.hpp
	${MYMONERO_CORE_CPP_SRC}/serial_bridge_utils.cpp
	${MYMONERO_CORE_CPP_SRC}/tools__ret_vals.hpp
	${MYMONERO_CORE_CPP_SRC}/tools__ret_vals.cpp
	#
	${MONERO_SRC}/cryptonote_basic/cryptonote_basic_impl.cpp
	${MONERO_SRC}/cryptonote_basic/account.cpp
	${MONERO_SRC}/cryptonote_basic/cryptonote_format_utils.cpp
	${MONERO_SRC}/crypto/crypto.cpp
	${MONERO_SRC}/crypto/hash.c
	${MONERO_SRC}/crypto/slow-hash-dummied.cpp
	${MONERO_SRC}/crypto/oaes_lib.c
	${MONERO_SRC}/crypto/crypto-ops.c
	${MONERO_SRC}/crypto/crypto-ops-data.c
	${MONERO_SRC}/crypto/keccak.c
	${MONERO_SRC}/crypto/chacha.c
	${MONERO_SRC}/crypto/random.c
	${MONERO_SRC}/crypto/aesb.c
	${MONERO_SRC}/crypto/tree-hash.c
	${MONERO_SRC}/crypto/hash-extra-blake.c
	${MONERO_SRC}/crypto/blake256.c
	${MONERO_SRC}/crypto/hash-extra-groestl.c
	${MONERO_SRC}/crypto/hash-extra-jh.c
	${MONERO_SRC}/crypto/hash-extra-skein.c
	${MONERO_SRC}/crypto/groestl.c
	${MONERO_SRC}/crypto/jh.c
	${MONERO_SRC}/crypto/skein.c
	${MONERO_SRC}/cryptonote_core/cryptonote_tx_utils.cpp
	${MONERO_SRC}/common/base58.cpp
	${MONERO_SRC}/common/threadpool.cpp
	${MONERO_SRC}/common/aligned.c
	${MONERO_SRC}/common/util.cpp
	${MONERO_SRC}/epee/src/hex.cpp
	${MONERO_SRC}/epee/src/string_tools.cpp
	${MONERO_SRC}/epee/src/memwipe.c
	${MONERO_SRC}/epee/src/mlocker.cpp
	${MONERO_SRC}/epee/src/wipeable_string.cpp
	${MONERO_SRC}/device/device.cpp
	${MONERO_SRC}/device/device_default.cpp
	${MONERO_SRC}/ringct/rctOps.cpp
	${MONERO_SRC}/ringct/rctTypes.cpp
	${MONERO_SRC}/ringct/rctCryptoOps.c
	${MONERO_SRC}/ringct/rctSigs.cpp
	${MONERO_SRC}/ringct/bulletproofs.cc
	${MONERO_SRC}/ringct/multiexp.cc
	${MONERO_SRC}/mnemonics/electrum-words.cpp
	${MONERO_SRC}/vtlogger/logger.cpp
	${MONERO_SRC}/contrib/libsodium/src/crypto_verify/verify.c
)
# needed for slow-hash
add_compile_options(-maes)

#Run through each source
foreach(testSrc ${TEST_SRCS})
    # extract the filename without an extension (NAME_WE)
    get_filename_component(testName ${testSrc} NAME_WE)
    add_executable(
        ${testName}
        ${testSrc}
        ${SRC_FILES}
    )
    target_link_libraries(
        ${testName}
        #
        OpenSSL::SSL
        ${Boost_LIBRARIES} 
    )
    set_target_properties(
        ${testName} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/build/products
    )
    add_test(
        NAME ${testName} 
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/products 
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build/products/${testName} --catch_system_error=yes
    )
endforeach(testSrc)
