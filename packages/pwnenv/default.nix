{pkgs,...}:
let 
py = pkgs.python3.withPackages (ps: with ps; [
  pwntools
  r2pipe
]);
in
pkgs.buildFHSEnv {
  name = "test-fhs-env";
  targetPkgs = pkgs: (with pkgs; [
    tmux
    neovim
    radare2
    py
  ]);
}
