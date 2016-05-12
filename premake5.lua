
solution "Building a Game Network Protocol"
    platforms { "x64" }
    includedirs { ".", "vectorial" }
    if not os.is "windows" then
        targetdir "bin/"
    end
    configurations { "Debug", "Release" }
    flags { "ExtraWarnings", "FloatFast" }
    exceptionhandling "Off"
    rtti "Off"
    configuration "Debug"
        flags { "Symbols" }
        defines { "DEBUG" }
    configuration "Release"
        optimize "Speed"
        defines { "NDEBUG" }

project "test"
    language "C++"
    kind "ConsoleApp"
    files { "test.cpp", "protocol2.h", "network2.h" }

project "001_reading_and_writing_packets"
    language "C++"
    kind "ConsoleApp"
    files { "001_reading_and_writing_packets.cpp", "protocol2.h" }

project "002_serialization_strategies"
    language "C++"
    kind "ConsoleApp"
    files { "002_serialization_strategies.cpp", "protocol2.h" }

project "003_packet_fragmentation_and_reassembly"
    language "C++"
    kind "ConsoleApp"
    files { "003_packet_fragmentation_and_reassembly.cpp", "protocol2.h" }

project "004_sending_large_blocks_of_data"
    language "C++"
    kind "ConsoleApp"
    files { "004_sending_large_blocks_of_data.cpp", "protocol2.h", "network2.h" }

project "005_packet_aggregation"
    language "C++"
    kind "ConsoleApp"
    files { "005_packet_aggregation.cpp", "protocol2.h", "network2.h" }

project "006_client_server"
    language "C++"
    kind "ConsoleApp"
    files { "006_client_server.cpp", "protocol2.h", "network2.h" }

if _ACTION == "clean" then
    os.rmdir "obj"
    if not os.is "windows" then
        os.execute "rm -rf bin"
        os.execute "rm -rf obj"
        os.execute "rm -f Makefile"
        os.execute "rm -f protocol2"
        os.execute "rm -f network2"
        os.execute "rm -f *.zip"
        os.execute "rm -f *.make"
        os.execute "rm -f test"
        os.execute "rm -f 001_reading_and_writing_packets"
        os.execute "rm -f 002_serialization_strategies"
        os.execute "rm -f 003_packet_fragmentation_and_reassembly"
        os.execute "rm -f 004_sending_large_blocks_of_data"
        os.execute "rm -f 005_packet_aggregation"
        os.execute "rm -f 006_client_server"
        os.execute "find . -name .DS_Store -delete"
    else
        os.rmdir "ipch"
		os.rmdir "bin"
		os.rmdir ".vs"
        os.execute "del /F /Q *.zip"
        os.execute "del /F /Q *.db"
        os.execute "del /F /Q *.opendb"
        os.execute "del /F /Q *.vcproj"
        os.execute "del /F /Q *.vcxproj"
        os.execute "del /F /Q *.sln"
    end
end

if not os.is "windows" then

    newaction
    {
        trigger     = "zip",
        description = "Zip up archive of this project",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            _ACTION = "clean"
            premake.action.call( "clean" )
            os.execute "zip -9r \"Building a Game Network Protocol.zip\" *.cpp *.h vectorial premake5.lua"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 test" == 0 then
                os.execute "./bin/test"
            end
        end
    }

    newaction
    {
        trigger     = "001",
        description = "Build example source for reading and writing packets",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 001_reading_and_writing_packets" == 0 then
                os.execute "./bin/001_reading_and_writing_packets"
            end
        end
    }

    newaction
    {
        trigger     = "002",
        description = "Build example source for serialization strategies",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 002_serialization_strategies" == 0 then
                os.execute "./bin/002_serialization_strategies"
            end
        end
    }

    newaction
    {
        trigger     = "003",
        description = "Build example source for packet fragmentation and reassembly",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 003_packet_fragmentation_and_reassembly" == 0 then
                os.execute "./bin/003_packet_fragmentation_and_reassembly"
            end
        end
    }

    newaction
    {
        trigger     = "004",
        description = "Build example source for sending large blocks of data",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 004_sending_large_blocks_of_data" == 0 then
                os.execute "./bin/004_sending_large_blocks_of_data"
            end
        end
    }

    newaction
    {
        trigger     = "005",
        description = "Build example source for packet aggregation",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 005_packet_aggregation" == 0 then
                os.execute "./bin/005_packet_aggregation"
            end
        end
    }

    newaction
    {
        trigger     = "006",
        description = "Build example source for client/server",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 006_client_server" == 0 then
                os.execute "./bin/006_client_server"
            end
        end
    }

end