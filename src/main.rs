// CS 510 HW 1 Gavin Megson
// Expanding example from Programming Rust, by Blandy and Orendorff

// Note that all the required functions are associative and commutative, and thus
// the arguments can be fed and processed iteratively in any order; therefore, 
// I've elected to create a generic function "callfn" in the functional style.


use std::io::Write;
use std::str::FromStr;


fn callfn(chosenfn: fn(u64, u64) -> u64, nums: &Vec<u64>) -> u64 {
	println!("{:?}",nums);
	assert!(nums.len() > 1); // other cases handled in main
	let mut n: u64 = nums[0];
	let mut m: u64 = nums[1];
	n = chosenfn(n, m);
	for i in nums.iter().skip(2) {
		m = *i;
		n = chosenfn(n, m);
	}
	n
}
		

// GCD and associated test from example
fn gcd(mut n: u64, mut m: u64) -> u64 {
	assert!(n != 0 && m != 0);
	while m != 0 {
		if m < n {
			let t = m; m = n; n = t;
		}
		m = m % n;
	}
	n
}

fn sum(n: u64, m: u64) -> u64 {
	n + m
}

fn product(n: u64, m: u64) -> u64 {
	n * m
}

fn lcm(n: u64, m: u64) -> u64 {
	assert!(n != 0 && m != 0);
	n * m / gcd(n, m)
}


#[test]
fn test_gcd() {
	assert_eq!(gcd(2*5*11*17, 3*7*13*19), 1);
	assert_eq!(gcd(2*3*5*11*17, 3*7*11*13*19), 3*11);
}

// I have not bothered to test sum, product, and lcm in isolation, due to their simplicity.
// They are implicitly checked in test_callfn.

#[test]
fn test_callfn() {
	let testnums = vec![2,3,5,7,13,23,30];
	let mut loopsum = 0;
	let mut loopprod = 1;
	for i in &testnums {
		loopsum += i;
		loopprod *= i;
	}
	assert_eq!(callfn(gcd, &testnums), 1);
	assert_eq!(callfn(sum, &testnums), loopsum);
	assert_eq!(callfn(product, &testnums), loopprod);
	assert_eq!(callfn(lcm, &testnums), 62790);

//	let mut testnum = testnums.split_off(1);
//	assert_eq!(callfn(gcd, &testnum), testnum[0]);
}

fn printusage () {
	writeln!(std::io::stderr(), "Type the number of the function, then \
			the arguments, separated by spaces. Choose from \
			the following functions:\n
			1) gcd
			2) sum
			3) product
			4) lcm\n\n").unwrap();
}

fn main() {
	let mut result = 0;

	let mut args = Vec::new();

	// Argument parsing mostly from example
	// Skipping the first two arguments and putting the rest into a vector of nums
	for arg in std::env::args().skip(1) {
		args.push(u64::from_str(&arg).expect("error parsing argument"));
	}

	// if no function specified, 
	if args.len() == 0 {
		printusage();
		std::process::exit(1);
	}

	// if only function specified, exit with success, as per instructions
	if args.len() == 1 {
		std::process::exit(0);
	}

	let text = [
		"GCD of ",
		"Sum of ",
		"Product of ",
		"LCM of ",
	];
	let mut toprint = 0;

	// determine which function is chosen by user
	let numbers = args.split_off(1);
	match args.first().unwrap() {
		1 => {
			result = callfn(gcd, &numbers);
			toprint = 0;
		}
		2 => {
			result = callfn(sum, &numbers);
			toprint = 1;
		}
		3 => {
			result = callfn(product, &numbers);
			toprint = 2;
		}
		4 => {
			result = callfn(lcm, &numbers);
			toprint = 3;
		}
		_ => {
			printusage();
		}
	}

	println!("{} {:?}: {}\n",text[toprint], numbers, result);
}
