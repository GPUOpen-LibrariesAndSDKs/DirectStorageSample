cmd /c BuildMediaUncompressed.bat
md .\BinaryOnlyBuild
robocopy .\ .\BinaryOnlyBuild\ /E /XF *.7z BuildCode.bat CreateBinaryOnlyDistribution.bat .git* /XD 4096 8192 src buildx build .git libs BinaryOnlyBuild buildx.old
"C:\Program Files\7-Zip\7z.exe" a BinaryOnlyBuildUncompressed.7z .\BinaryOnlyBuild\* 
