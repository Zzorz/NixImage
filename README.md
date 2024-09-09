# NixImage
Run nix packages anywhere, inspired by appimage.
# Quick start
1. You shuold have a working nix installed with `experimental-features = nix-command flakes`
2. Try with `nix bundle --bundler 'github:Zzorz/NixImage#niximage'  'nixpkgs#hello'`
3. extract `hello-$version.tar.gz` in current work directory and run `hello-$version/hello`
4. feel free to copy `hello-$version.tar.gz` to any machine

https://github.com/user-attachments/assets/0d52aa46-249a-4ac9-90eb-79c0b4e4ea19
# how it works

In summary, this tool is a combination of squashfuse and bubblewrap. Squashfs is used to pack the closure of drv.out into a single file, which is then mounted to a directory on the host system at runtime. Bubblewrap is used to create a new mount namespace and rebind the mounted Nix store to /nix/store.

Furthermore, during the build process, niximage will traverse all executable files under `${drv}/bin` and create the corresponding symbolic links. You can execute the files under `${drv}/bin/` via these symbolic links, or you can run the `${drv.name}.niximage` executable to get an interactive environment. In this environment, `${drv}/bin` will be prepended to the PATH environment variable.
