StepMania Texture Font Generator
================================

The `upstream-master` branch of this repo is copied verbatim from [the corresponding directory under the StepMania project](https://github.com/stepmania/stepmania/tree/master/src/Texture%20Font%20Generator).

Other branches contain modifications of this code. I seek to modify the software to make it usable outside of Windows. Since this code is MFC-based, this may not be a trivial effort.

One possibility is to use Visual Studio to remove the GUI (and with it much of the MFC-dependent code), replace it with a command-line interface, and ultimately simplify it enough for MinGW (which doesn't support MFC). At that point, there would still be some Windows-specific code, which could then be ported to cross-platform libraries instead.

In my opinion, having a GUI at all is secondary to having the software working at all, but I'll say that getting this working in GTK+, wxWidgets, and Qt (in that order) could be stretch goals.

Alternatively, once I've isolated the useful parts of the application, it might be simple enough to port to Java or Python.

License
-------

Copyright (c) 2003-2007 Glenn Maynard

Copyright (c) 2014-2015 Peter S. May

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
