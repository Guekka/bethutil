{
  autoconf,
  automake,
  cacert,
  cmake,
  cmrc,
  coreutils,
  fetchFromGitHub,
  fmt_9,
  gcc,
  git,
  glibc,
  gperf,
  lib,
  libtool,
  makeWrapper,
  m4,
  ninja,
  pkg-config,
  runtimeShell,
  stdenv,
  zip,
  zstd,
}
: let
  runtimeDeps = [
    autoconf
    automake
    cacert
    coreutils
    cmake
    gcc
    git
    ninja
    pkg-config
    zip
    zstd
  ];
in
  stdenv.mkDerivation rec {
    pname = "vcpkg";
    version = "b18b17865cfb6bd24620a00f30691be6775abb96";

    src = fetchFromGitHub {
      owner = "microsoft";
      repo = "vcpkg-tool";
      rev = "bedcba5172f5e4b91caac660ab7afe92c27a9895";
      hash = "sha256-XgYCa9aiG8lOraClfsFCk7U1qxvsfsIKbcBd2xBcNSw=";
    };

    vcpkg_src = fetchFromGitHub {
      owner = "microsoft";
      repo = "vcpkg";
      rev = "5d2a0a9814db499f6ba2e847ca7ab5912badcdbf";
      hash = "sha256-UPThRUq8N2nmUoFMUgfLqu1JM3pjCz6mVdR11Iig4kE=";
    };

    nativeBuildInputs = [
      cmrc
      fmt_9
      makeWrapper
    ];

    buildInputs = runtimeDeps;

    cmakeFlags = [
      "-DVCPKG_DEPENDENCY_EXTERNAL_FMT=ON"
      "-DVCPKG_DEPENDENCY_CMAKERC=ON"
    ];

    vcpkgScript = let
      out = placeholder "out";
    in ''
      #!${runtimeShell}
      vcpkg_hash=$(echo -n "${out}" | sha256sum | cut -f1 -d ' ')
      vcpkg_root_path="$HOME/.local/vcpkg/roots/$vcpkg_hash"
      if [[ ! -d $vcpkg_root_path ]]; then
        mkdir -p $vcpkg_root_path
        ln -s ${out}/share/vcpkg/{docs,ports,scripts,triplets,versions,LICENSE.txt} $vcpkg_root_path/
        ln -s ${out}/bin/vcpkg $vcpkg_root_path/
        touch $vcpkg_root_path/.vcpkg-root # need write access
      fi

      if [[ "$1" == "--root-for-nix-usage" ]]; then
        echo "$vcpkg_root_path"
        exit 0
      fi

      # Remove --vcpkg-root and --downloads-root from the command-line arguments
      args=()
      skip_next=false

      for arg in "$@"; do
        if [[ "$skip_next" == true ]]; then
          skip_next=false
          continue
        fi

        if [[ "$arg" == "--vcpkg-root" || "$arg" == "--downloads-root" ]]; then
          skip_next=true
          continue
        fi

        args+=("$arg")
      done

      export VCPKG_FORCE_SYSTEM_BINARIES=1
      exec ${out}/share/vcpkg/vcpkg \
        --vcpkg-root "$vcpkg_root_path" \
        --downloads-root "$vcpkg_root_path/downloads" \
        "''${args[@]}"
    '';

    passAsFile = ["vcpkgScript"];

    installPhase = ''
      cmake --build . --target=install --config=Release
      mkdir -p $out/share/vcpkg

      cp --preserve=mode -r ${vcpkg_src}/{docs,ports,scripts,triplets,versions,LICENSE.txt} $out/share/vcpkg
      mv $out/bin/vcpkg $out/share/vcpkg/vcpkg
      cp $vcpkgScriptPath $out/bin/vcpkg
      chmod +x $out/bin/vcpkg
      touch $out/share/vcpkg/vcpkg.disable-metrics
    '';

    postFixup = ''
      wrapProgram $out/bin/vcpkg --set PATH ${lib.makeBinPath runtimeDeps}
    '';
  }
