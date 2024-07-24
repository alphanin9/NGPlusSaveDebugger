## A tiny tool to debug New Game+ save metadata reading

As some users have encountered problems with [New Game+](https://github.com/alphanin9/CyberpunkNewGamePlus) not seeing perfectly valid saves, I've built a tiny CLI tool to help them with these issues, reusing NG+'s save metadata validation code.

Uses [simdjson](https://github.com/simdjson/simdjson) for JSON parsing.