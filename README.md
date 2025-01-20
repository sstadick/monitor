# monitor 

Yet another utility to watch for changes to files and run a command when they change. The command run is currently hard coded as "make" in the directory that it's watching.

> :warning: **Not for public use** This code is certainly riddled with bugs because it's the first C code I've ever written. Use at your own risk.

The impetus for this tool is that I wanted to get better at C. I couldn't get the vscode extension to show me error or warnings, so I figured I should just use vim anyways, and why not write a watcher to have in a splitscreen to recompile on changes in place of an LSP. So begins my "recreate my whole world" journey.


## Usage

```bash
> ./monitor -h
monitor <DIR> <EXTS>
	<EXTS> are a comma-delimited list like '.c,.py'
	A very limited file watcher and command runner.
	This will run 'make' when a file is changed, for each file changed.
	This will changdir to the passe dir and then scan the files ever tenth of a second
	using a naive sum of the file bytes as a checksum.
```

Example
```
> ./monitor . c,h,py
```
