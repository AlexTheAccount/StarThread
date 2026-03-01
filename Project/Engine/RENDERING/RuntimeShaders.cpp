#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>

namespace filesystem = std::filesystem;

namespace RENDERING::RENDERER_HELPERS::Utilities
{
    std::vector<char> ReadFileBinary(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) return {};
        size_t size = (size_t)file.tellg();
        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), size);
        return buffer;
    }

    bool CompileHLSLWithDXC(const std::string& dxcPath, const std::string& hlsl, const std::string& outSpv, const std::string& profile)
    {
        try
        {
            // Resolve absolute paths so the external process sees correct files
            filesystem::path outPath(outSpv);
            filesystem::path hlslPath(hlsl);
            filesystem::path absOut = filesystem::absolute(outPath);
            filesystem::path absHlsl = filesystem::absolute(hlslPath);

            // Ensure output directory exists
            if (absOut.has_parent_path())
            {
                filesystem::create_directories(absOut.parent_path());
            }

            // Print current working directory and resolved paths to help debugging
            std::string cwd = filesystem::current_path().string();
            printf("CWD: %s\n", cwd.c_str());
            printf("DXC input: %s\n", absHlsl.string().c_str());
            printf("DXC output: %s\n", absOut.string().c_str());

            // Build command using absolute paths
            std::string command = dxcPath + " -T " + profile + " -E main -spirv -Fo \"" + absOut.string() + "\" \"" + absHlsl.string() + "\"";

            std::string shellCommand = "cmd /C " + command + " 2>&1";
            printf("Running shell command: %s\n", shellCommand.c_str());

            FILE* pipe = _popen(shellCommand.c_str(), "r");
            if (!pipe)
            {
                printf("Failed to run dxc command: %s\n", command.c_str());
                return false;
            }

            std::string output;
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                output += buffer;
            }
            int result = _pclose(pipe);

            if (result != 0)
            {
                printf("DXC failed (exit %d)\nCommand: %s\nOutput:\n%s\n", result, command.c_str(), output.c_str());
                return false;
            }

            // print warnings/info
            if (!output.empty())
            {
                printf("DXC output:\n%s\n", output.c_str());
            }

            return true;
        }
        catch (...)
        {
            printf("Exception in CompileHLSLWithDXC\n");
            return false;
        }
    }

    std::vector<char> LoadOrCompileShader(const std::string& hlslPath, const std::string& spvPath, const std::string& profile, const std::string& dxcPath = "dxc")
    {
        try 
        {
            bool needCompile = true;
            if (filesystem::exists(spvPath) && filesystem::exists(hlslPath)) 
            {
                auto hlslTime = filesystem::last_write_time(hlslPath);
                auto spvTime = filesystem::last_write_time(spvPath);
                needCompile = (hlslTime > spvTime);
            }
            if (needCompile) 
            {
                if (!CompileHLSLWithDXC(dxcPath, hlslPath, spvPath, profile)) 
                {
                    printf("Failed to compile shader '%s' to '%s' using profile '%s'\n", hlslPath.c_str(), spvPath.c_str(), profile.c_str());
                    return {};
                }
            }
            printf("Loaded shader '%s' from '%s'\n", hlslPath.c_str(), spvPath.c_str());
            return ReadFileBinary(spvPath);
        }
        catch (...) 
        {
            printf("Exception occurred while loading or compiling shader '%s'\n", hlslPath.c_str());
            return {};
        }
    }
}