const { exec } = require("child_process");

const repositoryUrl = "https://github.com/zeon-studio/hugoplate";
const localDirectory = "./themes/hugoplate";
const foldersToFetch = ["assets", "layouts"];
const foldersToSkip = ["exampleSite"];

const ALLOWED_FOLDERS = new Set(foldersToFetch);
const excludePattern = foldersToSkip.join("|");

function fetchFolder(folder) {
  if (!ALLOWED_FOLDERS.has(folder)) {
    throw new Error(`Unexpected folder: ${folder}`);
  }
  exec(
    `curl -L ${repositoryUrl}/tarball/main | tar -xz --strip-components=1 ` +
      `--directory=${localDirectory} --exclude=$(curl -sL ${repositoryUrl}/tarball/main | ` +
      `tar -tz | grep -E "/(${excludePattern})/") */${folder}`,
  );
}

if (require.main === module) {
  foldersToFetch.forEach(fetchFolder);
}

module.exports = { fetchFolder };
