# SPDX-FileCopyrightText: © 2022 Melg Eight <public.melg8@gmail.com>
#
# SPDX-License-Identifier: MIT

# Generated by pip2nix 0.8.0.dev1
# See https://github.com/nix-community/pip2nix

{ fetchurl }:

self: super: {
  "DataProperty" = super.buildPythonPackage rec {
    pname = "DataProperty";
    version = "0.55.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/82/e9/de64e519166a70b3638f2749684758a3bbecc1f9c8b71a36d2abdbebc200/DataProperty-0.55.0-py3-none-any.whl";
      sha256 = "0ckbavdyyjbcj1n92amrsflbk1qd2c4kial7lcrjqjqgjmsr3wm8";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."mbstrdecoder"
      self."typepy"
    ];
  };
  "giturlparse" = super.buildPythonPackage rec {
    pname = "giturlparse";
    version = "0.10.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/a2/01/baa326ad32510acd3f08835e1b9f3c66170ddf7dfcc233640ff22d0ea619/giturlparse-0.10.0-py2.py3-none-any.whl";
      sha256 = "1mvjysc3wbppkj9zrkrbs8n2r6va7j8j5914ipx96c4w14x1mfh4";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [ ];
  };
  "mbstrdecoder" = super.buildPythonPackage rec {
    pname = "mbstrdecoder";
    version = "1.1.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/28/fb/62b091148cdce6373290be5ee23ac46caecbc637ecc2a92ceb84570d9652/mbstrdecoder-1.1.0-py3-none-any.whl";
      sha256 = "1vghmqd5hfcjrp68j7iyw2rr0q8m6c6liy51cci9n32y9g1l6d0m";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."chardet"
    ];
  };
  "mdx-truly-sane-lists" = super.buildPythonPackage rec {
    pname = "mdx-truly-sane-lists";
    version = "1.2";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/05/d8/50d108921125389b2853aab2a93a1dbcf469b52f2f8521bcf6a2410cc6e5/mdx_truly_sane_lists-1.2-py3-none-any.whl";
      sha256 = "1a8xiz3fmvmfdcqq7qiin1yzqh3b9v4sjxr3w423ah1iyc0gm2yc";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."markdown"
    ];
  };
  "multiprocessing-logging" = super.buildPythonPackage rec {
    pname = "multiprocessing-logging";
    version = "0.3.1";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/2a/3f/9a123bb0f98b37956651dbc86d8a8821b6267ab96603c8a8b47d333e7f03/multiprocessing_logging-0.3.1-py2.py3-none-any.whl";
      sha256 = "1qz23msli1rd2aarb4ysvscijw57fxpp4ad0a1ibmdsrz3qv0glx";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [ ];
  };
  "pychalk" = super.buildPythonPackage rec {
    pname = "pychalk";
    version = "2.0.1";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/bb/e6/f99c99aaf6c80ed7fcb39c3479871db45a2970ca9fe713a0e2ca0e5cb592/pychalk-2.0.1.tar.gz";
      sha256 = "0fby9rhdqbp34qfh6ajk6fgmckbjffgl9hi21nikb256dxgjfqzp";
    };
    format = "setuptools";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [
      self."pbr"
    ];
    propagatedBuildInputs = [
      self."six"
    ];
  };
  "pytablewriter" = super.buildPythonPackage rec {
    pname = "pytablewriter";
    version = "0.64.2";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/d0/82/093f5d0729c43c81c4bb54054f9523a75eafb855f6c4d44fe6978cec6695/pytablewriter-0.64.2-py3-none-any.whl";
      sha256 = "10p3cmdwnr6san2sldcm26h3lfz3rqk8cvx82d10hkgg83f1svf4";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."DataProperty"
      self."mbstrdecoder"
      self."pathvalidate"
      self."setuptools"
      self."tabledata"
      self."tcolorpy"
      self."typepy"
    ];
  };
  "tabledata" = super.buildPythonPackage rec {
    pname = "tabledata";
    version = "1.3.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/fb/b6/8692a0f168708e0f91e56a3aa6285cc4e8f74938d463ba92dfa90902e66f/tabledata-1.3.0-py3-none-any.whl";
      sha256 = "1wc0i0iirsal8yn4n210arh9wdb1j6735zl2cqkg5fsj2mbgl5i0";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."DataProperty"
      self."typepy"
    ];
  };
  "tcolorpy" = super.buildPythonPackage rec {
    pname = "tcolorpy";
    version = "0.1.2";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/29/6b/81e0c2e8b9ceed6d49d8e9807dd42ca4835e8c7f4ad4fb634e89a7eab949/tcolorpy-0.1.2-py3-none-any.whl";
      sha256 = "1g5x7nlqw6vvjsqmff9z1rx0qm7hhhzxk5zvr6is0kyiyk0sbcc8";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [ ];
  };
  "typepy" = super.buildPythonPackage rec {
    pname = "typepy";
    version = "1.3.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/04/a0/27827f2581b7f292422bcac55c326a2fe785dc28bb4e9de9accac88be769/typepy-1.3.0-py3-none-any.whl";
      sha256 = "16hyls3svcydayg9jw4nv7fllclfq3p5yjic2m467kv956c166fg";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."mbstrdecoder"
      self."packaging"
      self."python-dateutil"
      self."pytz"
    ];
  };
  "webpreview" = super.buildPythonPackage rec {
    pname = "webpreview";
    version = "1.6.0";
    src = fetchurl {
      url = "https://files.pythonhosted.org/packages/91/00/73ebd3d699bfc47f3265a63316917260460d665b45891537a581aa517ad0/webpreview-1.6.0-py3-none-any.whl";
      sha256 = "0d7lxjdc527prz4aqdh3431gx7j0gw3j295wyr0fyh1zy206k4dj";
    };
    format = "wheel";
    doCheck = false;
    buildInputs = [ ];
    checkInputs = [ ];
    nativeBuildInputs = [ ];
    propagatedBuildInputs = [
      self."beautifulsoup4"
      self."requests"
    ];
  };
}
