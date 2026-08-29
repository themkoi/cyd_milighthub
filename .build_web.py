from subprocess import check_output, CalledProcessError
import sys
import os
import platform

# Handle execution both within PlatformIO and directly via Python CLI
try:
    Import("env")
    IN_PLATFORMIO = True
except NameError:
    IN_PLATFORMIO = False


def log(message):
    """Print immediately without stdout buffering issues."""
    print(f"[build_web] {message}", flush=True)


def is_tool(name):
    cmd = "where" if platform.system() == "Windows" else "which"
    try:
        check_output([cmd, name])
        return True
    except Exception:
        return False


def build_web(*args, **kwargs):
    log("Starting web asset build check...")

    if not os.path.exists("web2"):
        log("ERROR: Directory 'web2' not found in project root.")
        return

    if not is_tool("npm"):
        log("WARNING: 'npm' command not found in system PATH. Skipping web build.")
        return

    original_dir = os.getcwd()
    npm_cmd = "npm.cmd" if platform.system() == "Windows" else "npm"

    try:
        os.chdir("web2")
        log(f"Switched directory to: {os.getcwd()}")

        log("Running 'npm install'...")
        install_output = check_output([npm_cmd, "install"], text=True)
        log(install_output)

        log("Running 'npm run build'...")
        build_output = check_output([npm_cmd, "run", "build"], text=True)
        log(build_output)

        log("Web assets built successfully.")

    except CalledProcessError as e:
        log(f"ERROR: Command failed with return code {e.returncode}")
        if e.output:
            log(f"Output:\n{e.output}")
        log("WARNING: Failed to build web package. Using existing pre-built files if available.")

    except Exception as e:
        log(f"ERROR: Unexpected error ({type(e).__name__}): {e}")
        log("WARNING: Failed to build web package.")

    finally:
        os.chdir(original_dir)


if IN_PLATFORMIO:
    # Hook into PlatformIO build system so logs stream correctly
    env.AddPreAction("buildprog", build_web)
else:
    # Allow manual standalone execution: python .build_web.py
    build_web()