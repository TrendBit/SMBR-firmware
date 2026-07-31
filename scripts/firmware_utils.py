import pathlib
import subprocess

from application_codes import Message, module_types

class FactoryException(Exception):
    pass

class FirmwareVersion:
    def __init__(self, major : int, minor : int, patch : int):
        self.major=major
        self.minor=minor
        self.patch=patch

    @classmethod
    def from_string(cls, string : str):
        split_string = string.split(".")
        
        if len(split_string) != 3:
            raise FactoryException("not a valid version")
        
        major = int(split_string[0])
        minor = int(split_string[1])
        patch = int(split_string[2])
        
        return FirmwareVersion(major, minor, patch)
        
    @classmethod
    def from_message(cls, message : Message):
        if len(message.data) != 6:
            raise FactoryException("incorrect response length")
        
        major = int.from_bytes(message.data[0:2], byteorder='big')
        minor = int.from_bytes(message.data[2:4], byteorder='big')
        patch = int.from_bytes(message.data[4:6], byteorder='big')
        
        return FirmwareVersion(major,minor,patch)

    @classmethod
    def from_file(cls, file_path : str):
        file = open(file_path)
        file_contents = file.read()
        for line in file_contents.split("\n"):
            if line.startswith("version: "):
                return FirmwareVersion.from_string(line.split(" ")[1])

        raise FactoryException("given file is missing version string")

    def to_string(self):
        return f"{self.major}.{self.minor}.{self.patch}"
    
    def __lt__(self, other):
        if self.major != other.major:
            return self.major < other.major
        elif self.minor != other.minor:
            return self.minor < other.minor
        else:
            return self.patch < other.patch
    
    def __eq__(self, other):
        return self.major == other.major and self.minor == other.minor and self.patch == other.patch

    def __str__(self):
        return self.to_string()

class Firmware:
    def __init__(self, file_path : str, module_name : str, version : FirmwareVersion, with_metadata : bool = True) -> None:
        self.module_name = module_name
        self.version = version
        self.file_name = file_path
        self.with_metadata = with_metadata

    def get_version_string(self):
        return f"{self.version}{"" if self.with_metadata else "?"}"

    @classmethod
    def from_old_file(cls, file_path: str, fallback_version : FirmwareVersion):
        module_name=""
        try:
            module_name = file_path.split("/")[-1].split(".",1)[0].lower().capitalize()
        except Exception:
            raise Exception("Invalid file name format")

        if module_types.get(module_name):
            return Firmware(
                file_path,
                module_name,
                fallback_version,
                with_metadata=False
            )
        else:
            raise Exception("unknown module name")
    
    @classmethod
    def from_file(cls, file_path : str):
        ps_strings = subprocess.run(["strings", file_path], capture_output=True, stdin=subprocess.DEVNULL)
        if ps_strings.returncode != 0:
            raise FactoryException(f"Unable to read metadata, invalid return code {ps_strings.returncode}")
        
        major = None
        minor = None
        patch = None
        module_name = None
        
        for line in ps_strings.stdout.splitlines():
            if not line.startswith(b"___INFO___"):
                continue
            cols = line.decode().split('|',2)
            if len(cols) != 3:
                continue
            
            field = cols[1].strip()
            try:
                match field:
                    case "version":
                        version_chunks = cols[2].strip()[1:-1].split(".")
                        major = int(version_chunks[0])
                        minor = int(version_chunks[1])
                        patch = int(version_chunks[2])
                    case "module name":
                        module_name = cols[2].strip()[1:-1].lower()
                    case _:
                        pass
            except Exception as e:
                print(f"WARNING: file {file_path} contains a field \"{field}\" doens not contain a valid value: {e}")
        
        if (module_name != None 
            and major != None 
            and minor != None 
            and patch != None):
            return Firmware(file_path,module_name,FirmwareVersion(major,minor,patch))
        else:
            raise FactoryException("missing metadata")
            
    def __str__(self):
        return f"{self.module_name} fw version: {self.get_version_string()}"


def load_firmwares_from_dir(dir_path : str, fallback_version : FirmwareVersion | None = None, verbose = False):
    if verbose:
        print("loading firmwares:")

    # locate firmware files
    available_firmware_files = [path for path in pathlib.Path(dir_path).glob("*.bin")]
    if not available_firmware_files:
        raise Exception("No firmware binaries found")

    if verbose:
        print("")
        print(f"checking files: \n{"\n".join([str(file) for file in available_firmware_files])}")

    available_firmwares : dict[str,list[Firmware]] = {}
    for file in available_firmware_files:
        firmware = None
        error = None
        try:
            firmware = Firmware.from_file(str(file))
        except Exception as e:
            error = e
            if fallback_version:
                try:
                    firmware = Firmware.from_old_file(str(file),fallback_version)
                except Exception as e2:
                    error = e2

        module_name=""
        if firmware:
            module_name = firmware.module_name.capitalize()
            if module_name not in module_types:
                firmware = None
                error=f"invalid module name: \"{module_name}\""

        if firmware:
            if module_name in available_firmwares:
                available_firmwares[module_name].append(firmware)
            else:
                available_firmwares[module_name] = [firmware]
        else:
            print(f"WARNING: {file} is not a valid firmware file: {error if error else "unknown error"}")

    if verbose:
        print("")
        print("loaded firmwares:")
        for module_name, module_firmwares in available_firmwares.items():
            print(f"{module_name}")
            for firmware in module_firmwares:
                print(f"   {firmware.get_version_string()}")
            print("")

    return available_firmwares