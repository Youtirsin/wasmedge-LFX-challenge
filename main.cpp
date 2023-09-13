#include <wasmedge/wasmedge.h>

#include <cstring>
#include <iostream>
#include <vector>

void show_help() {
  std::cout << "USAGE:\n"
               "tool [version] [run] [wasm path] [arguments]\n"
               "`version` is optional and prints the used wasmedge version; if this is given, you can ignore any other options.\n"
               "`run` is optional.\n"
               "`wasm path` is required; the relative and absolute path should be supported.\n"
               "`arguments` is optional; some of the hidden wasm applications may not have arguments.\n";
}

class Error : public std::exception {
 public:
  Error(const char* msg) : message(msg) {}

  const char* what() const noexcept { return message.c_str(); }

 private:
  std::string message;
};

class Config {
 public:
  Config() : cxt(WasmEdge_ConfigureCreate()) {
    if (!cxt) {
      throw Error("failed to create wasmedge configuration.");
    }
    WasmEdge_ConfigureAddHostRegistration(cxt, WasmEdge_HostRegistration_Wasi);
  }

  ~Config() { WasmEdge_ConfigureDelete(cxt); }

  WasmEdge_ConfigureContext* get() const { return cxt; }

 private:
  WasmEdge_ConfigureContext* cxt;
};

class Vm {
 public:
  Vm(const Config& config) : cxt(WasmEdge_VMCreate(config.get(), NULL)) {
    if (!cxt) {
      throw Error("failed to create wasmedge vm.");
    }
  }

  ~Vm() { WasmEdge_VMDelete(cxt); }

  void wasi_init(const std::vector<const char*>& args) {
    WasmEdge_ModuleInstanceContext* wasiModule =
        WasmEdge_VMGetImportModuleContext(cxt, WasmEdge_HostRegistration_Wasi);
    if (!wasiModule) {
      throw Error("failed to init wasi module.");
    }
    WasmEdge_ModuleInstanceInitWASI(wasiModule, args.data(), args.size(), NULL,
                                    0, NULL, 0);
  }

  void run(const std::string& filepath) {
    WasmEdge_String start = WasmEdge_StringCreateByCString("_start");
    WasmEdge_Result res = WasmEdge_VMRunWasmFromFile(cxt, filepath.c_str(),
                                                     start, NULL, 0, NULL, 0);
    WasmEdge_StringDelete(start);

    if (!WasmEdge_ResultOK(res)) {
      std::string err =
          std::string("error running start: ") + WasmEdge_ResultGetMessage(res);
      throw Error(err.c_str());
    }
  }

 private:
  WasmEdge_VMContext* cxt;
};

int main(int argc, char** argv) {
  if (argc == 1) {
    show_help();
    return 1;
  }

  if (strcmp(argv[1], "version") == 0) {
    std::cout << WasmEdge_VersionGet() << '\n';
    return 0;
  }

  int filenameIdx = 1, argsIdx = 2;
  if (strcmp(argv[1], "run") == 0) {
    if (argc == 2) {
      show_help();
      return 1;
    }
    filenameIdx = 2;
    argsIdx = 3;
  }

  try {
    std::string filepath(argv[filenameIdx]);
    std::vector<const char*> args = {filepath.c_str()};
    for (int i = argsIdx; i < argc; ++i) {
      args.push_back(argv[i]);
    }
    Config config;
    Vm vm(config);
    vm.wasi_init(args);
    vm.run(filepath);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
