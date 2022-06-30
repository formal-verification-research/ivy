# ivy

IVy is a research tool intended to allow interactive development of
protocols and their proofs of correctness and to provide a platform
for developing and experimenting with automated proof techniques. In
particular, IVy provides interactive visualization of automated
proofs, and supports a use model in which the human protocol designer
and the automated tool interact to expose errors and prove
correctness.

# Installation

## Linux

This version has only been installed on Ubuntu 18.04 and 20.04. For Ubuntu 20.04, see further instructions below.

### 1. Install prerequisites:
```
sudo apt-get install python python-pip g++ cmake python-ply python-pygraphviz git python-tk tix pkg-config libssl-dev
```

### 2. Get our copy of IVy with its submodules:
```
git clone --recurse-submodules https://github.com/formal-verification-research/ivy.git 
cd ivy
```

### 3. Build the submodules (this takes a while):
```
python build_submodules.py
```

### 4. Install into local Python:
```
sudo python setup.py install
```

## A note about Ubuntu 20.04
IVy relies heavily on Python2. Thus, take the following notes into consideration:

>I was able to get IVy to compile and run on deeplearning with Ubuntu 22.04 yesterday. There are a couple of caveats you should know about when trying to install it:
>
>Most of the python-* dependencies (aside from python2 itself) must be installed via pip. This includes tk, and the other ones listed. For some reason, on the newest version of Ubuntu, pip2 installed through python-pip has issues, This can be resolved using pypa's official get-pip script at https://github.com/pypa/get-pip (there is a specific one for python2, but I can't remember the url you have to curl to get it, although it should tell you).
>
>Then, because IVy expects the `python` executable to be `python2`, you have to create a symlink to it. If you type `which python2`, you will probably get /usr/bin/python2. If so, you will have to make sure /usr/bin/python does not exist, and then do `sudo ln -T /usr/bin/python /usr/bin/python2`.
>
>Finally, there is a C header that is missing when you try running build_submodules.py. I can't remember what the package is you have to install to get access to it, but if you google the header name it should tell you.
