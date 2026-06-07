Publishing to GitHub and ReadTheDocs
=====================================

This guide walks through the complete procedure for:

1. Creating a GitHub repository for the documentation source
2. Structuring the repo so ReadTheDocs can build it automatically
3. Connecting ReadTheDocs to GitHub for live CI/CD rebuilds on every push

The result is a publicly hosted documentation site that rebuilds
automatically every time you push changes to GitHub.

----

Part 1 — Prepare the Repository Structure
------------------------------------------

ReadTheDocs expects a specific layout. The docs project as built in this
guide already matches it:

.. code-block:: text

   shr_docs/                    ← repository root
   ├── .readthedocs.yaml        ← RTD build configuration (create this)
   ├── requirements.txt         ← Python dependencies (create this)
   ├── source/
   │   ├── conf.py              ← Sphinx configuration
   │   ├── index.rst            ← master document
   │   ├── *.rst                ← all content pages
   │   └── _static/
   │       ├── custom.css
   │       └── solution_overview.svg
   └── build/                   ← local build output (git-ignored)

Create ``.readthedocs.yaml``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This file tells ReadTheDocs exactly how to build your docs:

.. code-block:: bash

   cat > .readthedocs.yaml << 'REOF'
   version: 2

   build:
     os: ubuntu-22.04
     tools:
       python: "3.11"

   sphinx:
     configuration: source/conf.py
     fail_on_warning: false

   python:
     install:
       - requirements: requirements.txt
   REOF

Create ``requirements.txt``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   cat > requirements.txt << 'REOF'
   sphinx>=7.0
   sphinx-rtd-theme>=2.0
   REOF

Create ``.gitignore``
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   cat > .gitignore << 'REOF'
   # Sphinx build output — never commit this
   build/
   source/.doctrees/
   *.pyc
   __pycache__/
   .DS_Store
   *.swp
   *.swo
   REOF

----

Part 2 — Initialise the Git Repository
-----------------------------------------

.. code-block:: bash

   # Make sure you are in the shr_docs/ directory
   cd ~/shr_docs

   # Initialise git
   git init

   # Set your identity (if not already configured globally)
   git config user.name  "Luis Viveros"
   git config user.email "your.email@example.com"

   # Stage all source files
   git add .readthedocs.yaml requirements.txt .gitignore source/

   # Verify what will be committed — should NOT include build/
   git status

   # Initial commit
   git commit -m "Initial commit: SHR 10GigE + Bedrock V3000 documentation

   Complete solution guide covering:
   - SVS-Vistek SHR camera series (101–245 MP)
   - SolidRun Bedrock V3000 host platform
   - Vimba X 2026-1 SDK integration (C++)
   - Qt 6 trigger and acquisition application
   - GNSS/NMEA metadata injection via RS232

   Author: Luis Viveros, June 2026"

----

Part 3 — Create the GitHub Repository
----------------------------------------

**Option A — GitHub CLI (recommended):**

.. code-block:: bash

   # Install GitHub CLI if needed
   sudo apt install gh

   # Authenticate
   gh auth login

   # Create the repo and push in one step
   gh repo create shr-10gige-bedrock-docs \
       --public \
       --description "SHR 10GigE + Bedrock V3000 complete solution docs" \
       --source=. \
       --remote=origin \
       --push

**Option B — Manual via GitHub web:**

1. Go to https://github.com/new
2. Repository name: ``shr-10gige-bedrock-docs``
3. Description: ``SHR 10GigE + Bedrock V3000 complete solution documentation``
4. Set to **Public** (required for free ReadTheDocs)
5. Do **not** initialise with README, .gitignore, or license
6. Click **Create repository**
7. Then push from the terminal:

.. code-block:: bash

   # Add the GitHub remote (replace YOUR_USERNAME)
   git remote add origin https://github.com/YOUR_USERNAME/shr-10gige-bedrock-docs.git

   # Push to GitHub
   git push -u origin main

Verify the push succeeded:

.. code-block:: bash

   # Check remote
   git remote -v
   git log --oneline

----

Part 4 — Connect ReadTheDocs
------------------------------

**Step 1 — Create a ReadTheDocs account:**

Go to https://readthedocs.org and sign up (or log in) with your GitHub account.
Using GitHub login is strongly recommended — it simplifies repository access.

**Step 2 — Import the project:**

1. In ReadTheDocs, click **"Import a Project"**
2. Click **"Import from GitHub"**
3. Find ``shr-10gige-bedrock-docs`` in the list and click the **+** button
4. Review the settings:

   - **Name:** ``shr-10gige-bedrock-docs``
   - **Repository URL:** (auto-filled from GitHub)
   - **Default branch:** ``main``
   - **Language:** English

5. Click **"Next"**, then **"Build version"**

**Step 3 — Wait for the first build:**

ReadTheDocs will clone the repository, install ``requirements.txt``, and run
``sphinx-build``. The build log is visible in real time. A successful build
produces a live URL:

.. code-block:: text

   https://shr-10gige-bedrock-docs.readthedocs.io/en/latest/

**Step 4 — Verify the result:**

Open the URL and confirm:

- The intro page displays with the solution overview diagram
- All 6 parts appear in the left sidebar navigation
- Code blocks are syntax-highlighted
- External links are clickable

----

Part 5 — Ongoing Workflow
---------------------------

Every time you update the documentation, the workflow is:

.. code-block:: bash

   # 1. Edit RST files in source/
   nano source/gnss_full_implementation.rst

   # 2. Preview locally before pushing
   sphinx-build -b html source build/html -q
   xdg-open build/html/index.html

   # 3. Stage and commit
   git add source/
   git commit -m "Update GNSS implementation: add RTK notes"

   # 4. Push — ReadTheDocs rebuilds automatically within ~2 minutes
   git push

ReadTheDocs detects the push via a GitHub webhook (set up automatically
during import) and triggers a new build. No manual action required.

Useful git commands for documentation work
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   # See what has changed since the last commit
   git diff source/

   # See commit history
   git log --oneline --graph

   # Create a release tag (e.g. for a stable version snapshot)
   git tag -a v1.0 -m "Release 1.0 — initial complete solution guide"
   git push origin v1.0

   # Amend the last commit message (before pushing)
   git commit --amend -m "Better commit message"

   # Undo changes to a file (revert to last commit)
   git checkout -- source/gnss_intro.rst

ReadTheDocs Versioned Builds
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ReadTheDocs can build multiple versions of your docs from different
branches or tags. This is useful when the camera firmware or SDK version
changes:

1. In ReadTheDocs → **Admin** → **Versions**
2. Activate the ``v1.0`` tag you pushed
3. ReadTheDocs builds a ``/en/v1.0/`` URL alongside ``/en/latest/``

This lets users access documentation matching their installed SDK version.

----

Part 6 — Optional: Custom Domain
----------------------------------

To serve docs from your own domain (e.g. ``docs.yourdomain.com``):

1. In ReadTheDocs → **Admin** → **Domains**
2. Add your domain and note the CNAME target ReadTheDocs provides
3. In your DNS provider, add a CNAME record:

.. code-block:: text

   docs.yourdomain.com  →  CNAME  →  shr-10gige-bedrock-docs.readthedocs.io

4. ReadTheDocs automatically provisions an SSL certificate (Let's Encrypt)

----

Summary
--------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Step
     - Command / Action
   * - Prepare RTD config
     - Create ``.readthedocs.yaml`` and ``requirements.txt``
   * - Initialise git
     - ``git init`` → ``git add source/ ...`` → ``git commit``
   * - Create GitHub repo
     - ``gh repo create`` or GitHub web → push
   * - Connect RTD
     - readthedocs.org → Import from GitHub → Build
   * - Update docs
     - Edit RST → ``git add`` → ``git commit`` → ``git push``
   * - Tag a release
     - ``git tag -a v1.x`` → ``git push origin v1.x``

Useful links:

- ReadTheDocs: https://readthedocs.org
- ReadTheDocs documentation: https://docs.readthedocs.io
- GitHub: https://github.com
- GitHub CLI: https://cli.github.com
- Sphinx documentation: https://www.sphinx-doc.org
