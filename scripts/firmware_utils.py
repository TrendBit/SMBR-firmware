import subprocess
from application_codes import Message

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
    def __init__(self, file_path : str, module_name : str, version : FirmwareVersion) -> None:
        self.module_name = module_name
        self.version = version
        self.file_name = file_path
    
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
            
            
            match cols[1].strip():
                case "version":
                    version_chunks = cols[2].split(".")
                    major = int(version_chunks[0])
                    minor = int(version_chunks[1])
                    patch = int(version_chunks[2])
                case "module name":
                    module_name = cols[2].strip()[1:-1].lower()
                case _:
                    pass
        
        if (module_name != None 
            and major != None 
            and minor != None 
            and patch != None):
            return Firmware(file_path,module_name,FirmwareVersion(major,minor,patch))
        else:
            raise FactoryException("missing metadata")
            
    def __str__(self):
        return f"{self.module_name} fw version: {self.version}"