{ pkgs, ... }:
let
  py = pkgs.python3.withPackages (ps: with ps; [
    pwntools
    r2pipe
  ]);
in
pkgs.buildEnv {
  name = "pwn-env";
  paths = with pkgs; [
    vim
    radare2
    py
  ];
  pathsToLink = [ "/bin" ];
}
