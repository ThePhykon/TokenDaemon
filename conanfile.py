from conan import ConanFile
from conan.tools.cmake import cmake_layout


class EmailDaemon(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        # Füge nur die notwendigen Abhängigkeiten hinzu
        self.requires("libcurl/8.12.1")
        self.requires("zlib/1.2.13")  # Allgemeine Abhängigkeit für Zlib (nicht direkt für OpenSSL)
        self.requires("mbedtls/3.5.0")  # Mbed TLS als Alternative

    def configure(self):
        # Statische Bibliotheken verwenden
        self.options["*"].shared = False
        # TLS-Backend auf Schannel setzen für Windows
        self.options["libcurl"].with_ssl = False
    
    def imports(self):
        # Kopiere alle notwendigen Abhängigkeiten
        self.copy("*.dll", dst="dependencies/bin", src="bin")
        self.copy("*.so*", dst="dependencies/lib", src="lib")
        self.copy("*.dylib*", dst="dependencies/lib", src="lib")
        self.copy("*.h", dst="dependencies/include", src="include")
        self.copy("*.a", dst="dependencies/lib", src="lib")

    def layout(self):
        # Definiere das CMake-Projektlayout
        cmake_layout(self)