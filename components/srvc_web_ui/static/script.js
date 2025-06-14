
let counter = 0;

setInterval(() => {
	
	document.querySelector("#text").textContent = `Counter ${counter}`;
	counter++;
	
}, 1000);
