# MacOS Sonoma - Getting fio (fio, fio.contracts, fio.devtools, fio.test) built

#### With every major update of the MacOS comes an update to both XCode as well as the CLT and with that incompatibilities with previous versions as well as incompatibilities with external software. For instance MacOS Sonoma includes XCode 14.x with includes Clang 15, OpenSSL 3.x, etc which are incompatible with FIO.

### Supporting Links
Most options found online are oriented to a workstation (desktop/laptop), and default
to the latest XCode and CLT for the host OS without the option to install an older
version. The following option should apply to both a workstation, a virtual machine,
as well as our use case; an AWS MacOS instance, and has the ability to install an
older xcode/clt version.

#### Primary Option
https://stackoverflow.com/questions/68646946/how-to-install-xcode-app-via-terminal-only-without-using-gui
https://github.com/sebsto/xcodeinstall

#### Apple downloads site
https://developer.apple.com/download/all/

#### How to switch openssl version on MacOS using homebrew
https://gist.github.com/zulhfreelancer/4a609f65dfc396e395e5d5713fb3dd0f

#### Other
Download/Install xcode via UI
https://www.freecodecamp.org/news/how-to-download-and-install-xcode/
https://www.howtogeek.com/737510/how-to-download-and-install-older-versions-of-macos/
https://lifehacker.com/how-to-download-and-install-older-macos-versions-with-t-1839671161

## Disclaimer:
This process will install XCode which will include CLT (command line tools) items including clang. While this procedure will allow the build, deploy and execution of FIO, any conflicts/bad behavior caused by having different versions of XCode and CLT installed is unknown at this time

## Pre-reqs: homebrew
See https://brew.sh/

## Steps
1) Log into the MacOS machine and bring up a terminal

2) Check XCode and CLT Version (FYI step)

> brew config
```
HOMEBREW_VERSION: 4.3.1
ORIGIN: https://github.com/Homebrew/brew
HEAD: e0bc557e7b991cb23583679e1cf1c8a92b793aeb
Last commit: 8 weeks ago
Core tap HEAD: 0f0bcf8cd7710dbc5a9763b43c5955cc0952fefb
Core tap last commit: 8 weeks ago
Core cask tap origin: https://github.com/Homebrew/homebrew-cask.git
Core cask tap HEAD: 055b3ca3b16b923d063b13d590550e2553245435
Core cask tap last commit: 8 weeks ago
HOMEBREW_PREFIX: /usr/local
HOMEBREW_CASK_OPTS: []
HOMEBREW_MAKE_JOBS: 12
Homebrew Ruby: 3.3.1 => /usr/local/Homebrew/Library/Homebrew/vendor/portable-ruby/3.3.1/bin/ruby
CPU: dodeca-core 64-bit kabylake
Clang: 15.0.0 build 1500
Git: 2.39.3 => /Library/Developer/CommandLineTools/usr/bin/git
Curl: 8.6.0 => /usr/bin/curl
macOS: 14.5-x86_64
CLT: 15.3.0.0.1.1708487402 <== MacOS Command-line tools version
Xcode: N/A                 <== MacOS Xcode version (Not installed)
```
Note the Clang and Xcode versions above

3) Install Sebsto custom tap (to be able to install xcodeinstall app developed by Sebsto (Sebastien Stormacq). Note that
this action will update homebrew itself, incl its config, dependencies, etc. See https://github.com/sebsto/xcodeinstall

> brew tap sebsto/macos

4) Verify that output from above command includes;

```
'==> Tapping sebsto/macos'
..
'Tapped 1 formula (13 files, 21.7KB)'
```

5) Install Sebsto's xcodeinstall app (developed by Sebastien Stormacq)
> brew install xcodeinstall

6) Verify that output of brew install contains the following

```
==> Fetching sebsto/macos/xcodeinstall
..
==> Installing xcodeinstall from sebsto/macos
..
==> Running `brew cleanup xcodeinstall`...
```

7) Verify that your user has a sudoers file in /etc/sudoers.d
> cat /etc/sudoers.d/<user id>

```
# Give your_user_id sudo access
<user id> ALL=(ALL) NOPASSWD:ALL
```

The MacOS Xcode installable may be downloaded using the xcodeinstall app (requires authentication) or it may be downloaded manually
via the Apple downloads webpage. Regardless of which option is used, your Apple creds must be entered. In addition if MFA is enabled that will be required too.

8) Manual download of Apple MacOS XCode installable
    1) From the Apple Developers Site
        1) Go to https://developer.apple.com/download/all/ in your browser
        2) Log in using your Apple creds
        3) In the search bar enter Xcode 11.7
        4) Click 'View Details'
        5) Click the download arrow to download 'Xcode 11.7.xip'
    2) From the FIO AWS S3 bucket 
        1) Download via the browser by going to the FIO AWS S3 bucket at https://us-east-1.console.aws.amazon.com/s3/buckets/fioprotocol?region=us-east-1&bucketType=general&prefix=build-tools/&showversions=false, selecting the Xcode xip file and clicking download
        2) Download the xip file using wget, i.e. ```wget https://fioprotocol.s3.amazonaws.com/build-tools/Xcode+11.7.xip```
    3) Transfer the xip file to the respective machine (Note for my aws instance I use FileZilla)
    4) xcodeinstall will install the file from its download directory, however, this must be created manually if not using xcodeinstall for the download
      1) Run ```xcodeinstall list``` (disregard the authentication error)
      2) Move the app to the xcodeinstall download directory using the command; mv "Xcode 11.7.zip" ~/.xcodeinstall/download
9) Automated install of the Apple MacOS XCode installable using xcodeinstall
    1) ```xcodeinstall authenticate``` (may request a 2MFA code if enabled)
        1) Enter your Apple ID username:
        2) Enter your Apple ID password:
        3) If MFA is active: Two factors authentication is enabled, enter your 2FA code:
        4) Verify authentication has successfully executed (Note that this action just create an Apple session for the download and install)
    2) ```xcodeinstall download --name "Xcode 11.7.xip"```
    3) ```xcodeinstall install --name "Xcode 11.7.xip"```

10) Verify the output contains the following
```
Installing...
[1/6] Expanding Xcode xip (this might take a while)
[2/6] Moving Xcode to /Applications
[3/6] Installing additional packages... XcodeSystemResources.pkg
[4/6] Installing additional packages... CoreTypes.pkg
[5/6] Installing additional packages... MobileDevice.pkg
[6/6] Installing additional packages... MobileDeviceDevelopment.pkg
[ OK ]
✅ file:///Users/ec2-user/.xcodeinstall/download/Xcode_11.5.xip installed
```
11) Accept Xcode license
> sudo xcodebuild -license accept

12) Verify config
> brew config
```
HOMEBREW_VERSION: 4.3.9
ORIGIN: https://github.com/Homebrew/brew
HEAD: 1c98a33a376e5b35ebc801d0f7d6ba7b9eb118ac
Last commit: 6 days ago
Core tap JSON: 09 Jul 14:29 UTC
Core cask tap JSON: 09 Jul 14:29 UTC
HOMEBREW_PREFIX: /usr/local
HOMEBREW_CASK_OPTS: []
HOMEBREW_MAKE_JOBS: 12
Homebrew Ruby: 3.3.3 => /usr/local/Homebrew/Library/Homebrew/vendor/portable-ruby/3.3.3/bin/ruby
CPU: dodeca-core 64-bit kabylake
Clang: 11.0.3 build 1103
Git: 2.24.3 => /Applications/Xcode.app/Contents/Developer/usr/bin/git
Curl: 8.4.0 => /usr/bin/curl
macOS: 14.3-x86_64
CLT: 15.1.0.0.1.1700200546 <=== MacOS Command-line tools version
Xcode: 15.0                <=== MacOS Xcode version
```
13) Verify clang
> which clang
> clang --version
```
Apple clang version 11.0.3 (clang-1103.0.32.62)
```
14) Verify installed openssl versions
> ls -al /usr/local/opt/openssl*
```
lrwxr-xr-x  1 ec2-user  admin  25 May 28 10:12 /usr/local/opt/openssl -> ../Cellar/openssl@3/3.3.0
lrwxr-xr-x  1 ec2-user  admin  25 May 28 10:12 /usr/local/opt/openssl@3 -> ../Cellar/openssl@3/3.3.0
lrwxr-xr-x  1 ec2-user  admin  25 May 28 10:12 /usr/local/opt/openssl@3.3 -> ../Cellar/openssl@3/3.3.0
```
15) Install openssl@1.1 if not listed above
> brew install openssl@1.1
```
==> Downloading https://formulae.brew.sh/api/formula.jws.json
############################################################################################################################################################################################################################ 100.0%
==> Downloading https://formulae.brew.sh/api/cask.jws.json
############################################################################################################################################################################################################################ 100.0%
Warning: openssl@1.1 has been deprecated because it is not supported upstream!
==> Downloading https://ghcr.io/v2/homebrew/core/openssl/1.1/manifests/1.1.1w
############################################################################################################################################################################################################################ 100.0%
==> Fetching openssl@1.1
==> Downloading https://ghcr.io/v2/homebrew/core/openssl/1.1/blobs/sha256:0b560cead0f34ec152b4d12b0f27081ded5b988a5c4aecb76c415ac760435ca9
############################################################################################################################################################################################################################ 100.0%
==> Pouring openssl@1.1--1.1.1w.sonoma.bottle.tar.gz
==> Caveats
A CA file has been bootstrapped using certificates from the system
keychain. To add additional certificates, place .pem files in
  /usr/local/etc/openssl@1.1/certs

and run
  /usr/local/opt/openssl@1.1/bin/c_rehash

openssl@1.1 is keg-only, which means it was not symlinked into /usr/local,
because this is an alternate version of another formula.

If you need to have openssl@1.1 first in your PATH, run:
  echo 'export PATH="/usr/local/opt/openssl@1.1/bin:$PATH"' >> ~/.zshrc

For compilers to find openssl@1.1 you may need to set:
  export LDFLAGS="-L/usr/local/opt/openssl@1.1/lib"
  export CPPFLAGS="-I/usr/local/opt/openssl@1.1/include"
==> Summary
🍺  /usr/local/Cellar/openssl@1.1/1.1.1w: 8,102 files, 18.5MB
==> Running `brew cleanup openssl@1.1`...
```

16) Put openssl 1.1 in PATH (before everything else) and set compile flags
> vi ~/.zshrc
```
PATH="/usr/local/opt/openssl@1.1/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/openssl@1.1/lib"
export CPPFLAGS="-I/usr/local/opt/openssl@1.1/include"
```

17) Install pyenv and python 2.7.18 (Must be done prior to installing older Xcode)
    1)  Install pyenv (see https://github.com/pyenv/pyenv?tab=readme-ov-file)
> brew install pyenv
```
==> Downloading https://ghcr.io/v2/homebrew/core/pyenv/manifests/2.4.7
############################################################################################################################################################################################################################ 100.0%
==> Fetching pyenv
==> Downloading https://ghcr.io/v2/homebrew/core/pyenv/blobs/sha256:225c342126303406c75839014db0082d138f500241d712c39ea7ecb4a01217d6
############################################################################################################################################################################################################################ 100.0%
==> Pouring pyenv--2.4.7.sonoma.bottle.tar.gz
▒~_~M▒  /usr/local/Cellar/pyenv/2.4.7: 1,215 files, 3.5MB
==> Running `brew cleanup pyenv`...
```

18) Update shell config (copy and paste this into terminal window)
```
echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.zshrc
echo '[[ -d $PYENV_ROOT/bin ]] && export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.zshrc
echo 'eval "$(pyenv init -)"' >> ~/.zshrc
```

19) Install python 2.7.18
> pyenv install 2.7.18

20) Make both python2 and python3 (default OS version) available
> pyenv global system 2.7.18

21) Build FIO
> cd ~/repos/fio/scripts
```
./fio_build.sh
```

22) Install FIO
```
./fio_install.sh
```
