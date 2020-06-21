def load_shader_source(source: str) -> str:
    with open(source) as source_file:
        return source_file.read()
