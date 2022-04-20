

# About

This folder contains Google Apps Script designed to be used within a container of Google Sheets table listing a Device Association Tree (DAT). The script provides menu button "Export to json" which displays a dialog window with the DAT data in the json form, along with a "Save" button to download it.


# Files

-   **`Code.js`:** The javascript code implementing all the logic.
-   **`appsscript.json`:** Google Cloud Platform configuration for the script. Do not edit unless knowing well what you are doing.
-   **`README.md`, `README.txt`:** Information about the script in two different formats for convenience.
-   **`spreadsheets.txt`:** The list of Google Apps Script projects this script should be pushed to after changes. See [Adding a new Google Sheet to the exporter installation list](#orgb9c6a42) section.
-   **`spreadsheets-install.sh`:** The bash script automating Google script installation. See [Clasp method](#org4ddebec) section.
-   **`.gitignore`:** The installation process may leave `.clasp.json` file, which should not be git tracked, and is thus included in the directory-local `.gitignore` file.


# Installation

For the script to be available in a particular spreadsheet it must be installed there. There are two ways to do that: "clasp" and "copy-paste".

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">&#xa0;</th>
<th scope="col" class="org-left">Clasp method</th>
<th scope="col" class="org-left">Copy-paste method</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">Interface</td>
<td class="org-left">CLI</td>
<td class="org-left">Web GUI</td>
</tr>


<tr>
<td class="org-left">Repository</td>
<td class="org-left">Tracked</td>
<td class="org-left">Untracked</td>
</tr>


<tr>
<td class="org-left">Tools</td>
<td class="org-left">Requires `npm`, `clasp`</td>
<td class="org-left">Zero dependencies</td>
</tr>


<tr>
<td class="org-left">Automation</td>
<td class="org-left">Bulk installation</td>
<td class="org-left">One spreadsheet at a time</td>
</tr>
</tbody>
</table>

As a general rule: use "clasp" method as a first choice and a "copy-paste" method as an emergency fallback.


<a id="org4ddebec"></a>

## Clasp method

1.  Make sure you have `Node.js` installed

        sudo npm install n -g
        sudo n latest

2.  Make sure you have `clasp` installed.

        sudo npm i @google/clasp -g

    `clasp` is a tool for pushing / pulling the code on local file system to / from a Google Apps Script project. For the quick overview of its basic functionalities see <https://codelabs.developers.google.com/codelabs/clasp/#0>. Note that you won't use it directly, but indirectly through the `spreadsheets-install.sh` program. See below.

3.  The script will be uploaded to all the Google Apps projects listed in `spreadsheets.txt` file. Make sure you have access to the google account X  having editing rights to ther containers (the documents they are bound to). If not sure you can check it per each link.

    1.  Open the link in a browser.
    2.  Click "Overview" (the "(i)" icon on the left pane).
    3.  Click the link under "Container" label.
    4.  Click "Share" button and locate yourself ("&#x2026; (you)") on the list.
    5.  See your role on the right whether it's "Owner" or "Editor".

    If all the steps succeded then uploading the script to this project shoud work. Otherwise it will be omitted.

4.  Make sure you have enabled Apps script api for the account X: <https://script.google.com/home/usersettings>.

5.  Log in to the account X using `clasp`:

        clasp login

6.  Run the installation script:

        ./spreadsheets-install.sh

    Warning: the installation will create a mirror of the `.js` and `.html` files contained in this folder on each Google Apps project side. That includes deleting the files which AREN'T listed.

7.  For the change to be visible in the spreadsheets a page reload may be necessary.


## Copy-paste method

1.  Open the target Google Sheets document. You must have editing rights ("Editor" or "Owner" role).

2.  In the main menu click `⟦Extensions⟧` → `⟦Apps Script⟧`. A new tab should open with `Code.gs` file selected and an editor containing a sample script with

        function myFunction() {

        }

    If something different is displayed then the spreadsheet already has some script defined. Consult the owner of the file to not erase someone's work.

3.  Delete the sample function.

4.  Copy the contents of `Code.js` file in this folder.

5.  Paste it in the online editor and save `⟦Ctrl + s⟧`.

6.  Switch to the spreadsheet tab and reload it.

7.  A new button `⟦Export⟧` in the main menu should appear. It may take a few seconds to load.


<a id="orgb9c6a42"></a>

# Adding a new Google Sheet to the exporter installation list

Adding a spreadsheet to the installation list allows for easy code pushing described in [Clasp method](#org4ddebec) and keeps git-controlled track of all the documents using the script. To add a new document to the set:

1.  Open the target Google Sheet.

2.  In the main menu click `⟦Extensions⟧` → `⟦Apps Script⟧`. (The `⟦Extensions⟧` menu may not be visible if you don't have editing rights.) A new tab should open.

3.  Copy the URL.

4.  Add it to the `spreadsheets.txt` file in a new line.