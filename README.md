kvfs
====
Goals:	
	local (cache) file system
		optimized for large directories of small files
		delivering ultra-high performance
	ultimate persistance is NAS or cloud
		where directories are up/down loaded as a single object
		providing efficient transfers and storage

Concept:
	a FUSE on top of a user-mode key value store
	tarballs as the NAS/cloud representation

Index:
	Docs	... documentation
	Notes	... design notes
	
	
Build Instructions:

1. Creating a Build Machine or Build VM on AWS:
-----------------------------------------------
If you don't already have a built instance created, you can create one via the Amazon Web Services console.
Cameron can can set up an account for you, which you'll access at https://cbfs.signin.aws.amazon.com/console
Currently, your login will be your first name (i.e. parascale user name).

Once logged in to the AWS console, select "EC2", then "Launch Instance".

Select the Ubuntu Server option (it will be at some recent Ubuntu build level).

You'll then need to choose the size of your machine. "t2.micro" should be sufficient for basic build-and-test machines.
Once you've select your desired machine type and OS, click "Review and Launch", then "Launch".

If this is your first time creating an instance, you'll be offered the chance to create a key-pair. Do so, download it and store it in a safe place (e.g. your parascale directory under your home on whatever machine you're using to access AWS).

Once you've downloaded the key-pair (a .pem file), click "Launch Instance".

The instance will take a few minutes to install and start. Once it's done, you can find it via the EC2 management console (you should see "1 Running Instances". Click on this, then click "Connect" for an explanation of how to connect to your instance. Suggest that you create a script which will ssh into your machine so you don't have to think about these details too often. Also, n.b. - the key-pair (.pem) file may need a "chmod go-rw" (ssh wants to make sure not publicly readable).

You should now be able to ssh into your build-test machine.


2. How to build from source:
----------------------------
Once you're logged into your build-test machine, you'll want to check out our tree. Click on the "Clone or download" button (you should see it above if you're viewing this file via github). Copy the git URL, then, on your new instance, create a directory ("kvfs_master"), cd into it, then do:

$ git clone https://github.com/ray1967/kvfs.git

(or whatever git url was copied when you clicked on the button).

Now you need to install some packages on your new machine. Run ./ubuntu-setup.sh at the top of the tree that git cloned.

$ ./ubuntu-setup.sh

Once packages are all installed, you can perform a build by running the following script:

$ ./test/build.sh

Note: rclone configuration to S3 bucket CBFS is done by build.sh copying a predefined config file to your $HOME. We can all use the same credentials to keep things simple and automatic for now.

3. Running basic sanity tests:
------------------------------
Run some basic end-to-end tests by:

cd test ; ./test_basic.sh

This should build without error and should report that it has run a number of tests. If everything is working correctly, you should see something like:

	12 PASSED:

	... (list of tests)...

	0 FAILED:

	---------

4. OPTIONAL: (not required to run) Note on rclone configuration if you want to configure manually or add other cloud targets:
---------------------------------------------------------------------------------------------
If the rclone has never been configured, you will need to configure the rclone by:
rclone config

The config shell will step you through.
1. Choose New remote option
2. Enter the Name of remote endpoint (e.g. s3)
3. Choose Remote endpoint type (e.g. 2 for s3)
4. Choose how it picks up the credential information (Choose option 1 to write the access_key_id and secrete)
5. Enter Your access key id
6. Enter Your Secret access key
7. Region to connect to. (Type enter to use the default)
8. Location constraint (Type enter to use the default)
9. Access control (Type enter to use the default)
10. Encryption.  (Type 1 for no encryption)
11. Storage class (Type enter to use the default)
12. Confirm your configuration


