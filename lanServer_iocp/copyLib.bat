
mkdir lanServer

robocopy headers lanServer/headers
robocopy release lanServer *.pdb
robocopy release lanServer *.lib

pause