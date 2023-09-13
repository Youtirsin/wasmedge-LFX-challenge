.PHONY:
test: tool add.wasm
	./tool version
	./tool add.wasm 3 4
	./tool run add.wasm  5 6

tool:
	g++ -o tool main.cpp -lwasmedge

add.wasm:
	emcc -o add.wasm add.c

.PHONY:
clean:
	rm tool
