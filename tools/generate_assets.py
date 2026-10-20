import os

def read_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

def main():
    html = read_file("web/index.html")
    css = read_file("web/style.css")
    js = read_file("web/app.js")

    content = f"""#pragma once

namespace EmbeddedAssets {{
    inline const char INDEX_HTML[] = R"rwbench_html({html})rwbench_html";
    inline const char STYLE_CSS[] = R"rwbench_css({css})rwbench_css";
    inline const char APP_JS[] = R"rwbench_js({js})rwbench_js";
}}
"""
    with open("src/embedded_assets.hpp", "w", encoding="utf-8") as f:
        f.write(content)
    print("src/embedded_assets.hpp generated successfully")

if __name__ == "__main__":
    main()
