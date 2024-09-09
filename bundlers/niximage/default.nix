{ pkgs, stdenv, writeScriptBin, ... }: drv:
let
  entry = stdenv.mkDerivation {
    name = drv.name+".niximage";
    src = ./.;
    buildInputs = [ pkgs.musl ];
    wrappedDrv = drv;
    configurePhase = ''
      substituteAllInPlace incbin.c
      cp ${pkgs.pkgsStatic.bubblewrap}/bin/bwrap .
      cp ${pkgs.pkgsStatic.squashfuse}/bin/squashfuse fuse
      chmod +w bwrap fuse
      strip -s bwrap fuse
    '';
    buildPhase = ''
      gcc -static -s *.c -o incbin
    '';
    installPhase = ''
      cp incbin $out
    '';

  };
in
stdenv.mkDerivation {
  name = drv.name+".tar.gz";
  closureInfo = pkgs.closureInfo { rootPaths = [ drv ]; };
  bundledDrv = drv;
  src = ./.;
  buildPhase= ''
    bin_path=${drv.name}
    mkdir -p $bin_path

    tar cf - $(< $closureInfo/store-paths) | ${pkgs.squashfsTools}/bin/mksquashfs - nix.img -comp zstd -no-recovery -all-root -tar -tarstyle
    
    cp ${entry} $bin_path/${entry.name}
    chmod +w $bin_path/${entry.name}
    cat nix.img >> $bin_path/${entry.name}
    chmod -w $bin_path/${entry.name}


    cd $bin_path
    for each in $(ls ${drv}/bin);do
      base=$(basename $each)
      if [ "${entry.name}" != $base ];then
        ln -s ${entry.name} $each 
      fi
    done
    cd ../
  '';
  installPhase = ''
    tar czf $out ${drv.name}
  '';
}

