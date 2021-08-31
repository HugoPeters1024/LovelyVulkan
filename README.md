## Development

### CoC + ccls in neovim

Make a symlink to the compile commands: `ln -s ./cmake-build-Debug/compile_commands.json .`

Create a .ccls file:

```
clang

# add this to support `.h` files as C++ headers
%h -x c++-header
%hpp -x c++-header

-I./include
-I./external/glm/glm
-I./external/glfw/include
-I./external/vks/vks/
-I./external/imgui/imgui/
-I./external/spdlog/include
-I/usr/bin/include
-std=c++17
```



