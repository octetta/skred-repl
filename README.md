<img src="logo.png" width="500">

# skred-repl

## Triggering a Multi-Platform Release

The repository is configured with a GitHub Actions workflow that automatically builds and packages binaries for Linux, macOS, and Windows. 

You can trigger a new release in one of two ways:

### Option 1: Using git tags (Recommended)
Tag your commit with a version number starting with `v` and push the tag to GitHub. The workflow will automatically run and publish a release.
```bash
git tag v0.1.2
git push origin v0.1.2
```

### Option 2: Manual Trigger via GitHub UI
If you prefer not to use git tags, you can manually trigger the release workflow from your browser:
1. Go to the **Actions** tab on your GitHub repository.
2. Click on **One-Click Multi-Platform Release** in the left sidebar.
3. Click the **Run workflow** dropdown on the right side.
4. Enter the version number you want to name the release (e.g., `v0.1.2`) and click the green **Run workflow** button.

Once the workflow finishes running, the compiled binaries and their checksums will automatically appear under the **Releases** section on the right side of the repository's main page.
