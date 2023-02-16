# Adding a new static library (in VS2022)
1. File -> New -> New Project
2. Static Library (C++)
3. Give an appropriate name, change Solution to "Add to solution", change location to "...\Async-Engine\Libs\"
4. Click Create
5. In whatever project you want to include it in:
	a. Right-click project -> Add -> Reference...
	b. Tick your new library
	c. OK
6. Add "$(ProjectDir)" to the library's Additional Include Directories, so that you can #include "pch.h" in subdirectories
https://stackoverflow.com/questions/12434123/how-to-include-the-stdafx-h-from-the-root-directory