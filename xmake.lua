local project_name = "rtsyn-measurement-tool"
local project_xmake_repo = "rtsyn-xmake-repo"

set_license("GPL-3.0-or-later")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")
set_defaultmode("release")
if is_mode("release") then
	set_optimize("fastest")
	set_strip("all")
	set_symbols("hidden")
end

option("valgrind", { default = false, description = "Run tests with Valgrind" })
option("tests", { default = true, description = "Build tests" })

set_languages("c23")
if has_config("tests") then
	add_requires("gtest")
	add_requires("rtsyn-test-utils")
end

local workspace = os.getenv("RTSYN_WORKSPACE")
if workspace then
	local repository_dir = path.join(workspace, project_xmake_repo)
	add_repositories(project_xmake_repo .. " " .. repository_dir)
else
	add_repositories(project_xmake_repo .. " https://github.com/seregioo/" .. project_xmake_repo .. ".git")
end

target(project_name)
set_kind("static")
add_files("src/*.c")
add_includedirs("include", { public = true })
add_includedirs("src")
add_headerfiles("include/(rtsyn/**.h)")

if has_config("tests") then
	target("tests/measurement_tool-tests")
	set_kind("binary")
	if has_config("valgrind") then
		add_rules("@rtsyn-test-utils/valgrind")
	end
	add_deps(project_name)
	add_packages("gtest")
	add_links("gtest_main")
	add_includedirs("src")
	add_files("tests/measurement_tool.cpp")
	add_tests("measurement-tool")
end
