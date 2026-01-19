Using llvm clang:
- much more up to date than Apple xcode clang, which is many versions behind
- more portable than MSVC and available on mac, windows, any linux and beyond

the homebrew install, which is the ONLY binary installer available for mac (indeed, for any os) is broken
- why?  llvm developers are either lazy, don't want to do lots of builds which they are the most qualified to do and can fully automate so once setup--there is no effort.  Or they really don't want to help people who may not want to build themselves.

how is the homebrew install broken:
- they want you to still be able to use Apple xcode clang:  well, you can--it's at a different path
- they want to make sure that if you do use xcode clang you get xcode clang's libraries: well, you will because xcode knows nothing about any weird homebrew keg paths within /opt.  xcode couldnt' find those libraries if it wanted to, which it certainly doesn't.

how to fix it easily:
- just create an llvm clang++ config at `~/.config/clang`
- it contains the following:
```# LLVM libc++ configuration for Homebrew
-L/opt/homebrew/opt/llvm/lib/c++
-L/opt/homebrew/opt/llvm/lib/unwind
-Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++
-lunwind
```

Note that the problem can also be fixed with xmake directives for a given target:
```lua
add_linkdirs("/opt/homebrew/opt/llvm/lib/c++")
add_rpathdirs("/opt/homebrew/opt/llvm/lib/c++")
```