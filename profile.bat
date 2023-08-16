set NumRuns=2
set DelayBetweenRuns=10
set AdditionalCmdLineOptions=,"stagingbuffersize":536870912,"profileseconds":500.0,"presentationMode":0, "cameraspeed":0.5

FOR /f "tokens=* delims=" %%A in ('bin\timestamp') do @set "ds_ts=%%A"

msinfo32.exe /REPORT "bin\msinfo_%ds_ts%.txt"
dxdiag /whql:off /t "bin\dxdiag_%ds_ts%.txt"

call BuildMediaUncompressed.bat&&Timeout /T %DelayBetweenRuns% /NOBREAK

for /l %%N in (1 1 %NumRuns%) do call RunNoDirectStorage.bat %AdditionalCmdLineOptions%&&Timeout /T %DelayBetweenRuns% /NOBREAK

for /l %%N in (1 1 %NumRuns%) do call RunDirectStorage.bat %AdditionalCmdLineOptions%&&Timeout /T %DelayBetweenRuns% /NOBREAK


call BuildMediaCompressed.bat &&Timeout /T %DelayBetweenRuns% /NOBREAK

for /l %%N in (1 1 %NumRuns%) do call RunDirectStorage.bat %AdditionalCmdLineOptions%&&Timeout /T %DelayBetweenRuns% /NOBREAK

for /l %%N in (1 1 %NumRuns%) do call RunDirectStorageCPUDecompression.bat %AdditionalCmdLineOptions%&&Timeout /T %DelayBetweenRuns% /NOBREAK

