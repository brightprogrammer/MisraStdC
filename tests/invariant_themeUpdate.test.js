jest.mock("child_process");
const { exec } = require("child_process");
const { fetchFolder } = require("../Docs/scripts/themeUpdate.js");

describe("fetchFolder allow-list guard", () => {
  beforeEach(() => jest.clearAllMocks());

  test.each(["assets", "layouts"])(
    'allowed folder "%s" calls exec once with correct path',
    (folder) => {
      fetchFolder(folder);
      expect(exec).toHaveBeenCalledTimes(1);
      expect(exec.mock.calls[0][0]).toContain(`*/${folder}`);
    },
  );

  test.each([
    "theme; rm -rf /",
    "$(whoami)",
    "`id`",
    "valid-theme-folder",
    "theme folder; echo hacked",
    "../etc/passwd",
  ])(
    'rejects disallowed input "%s" before exec is called',
    (folder) => {
      expect(() => fetchFolder(folder)).toThrow("Unexpected folder");
      expect(exec).not.toHaveBeenCalled();
    },
  );
});
