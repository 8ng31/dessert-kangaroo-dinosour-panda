// JavaScript to toggle password visibility
const togglePassword = document.querySelector('#togglePassword');
const passwordInput = document.querySelector('#password');

togglePassword.addEventListener('click', function () {
  // Toggle the type attribute of the password input between 'password' and 'text'
  const type = passwordInput.getAttribute('type') === 'password' ? 'text' : 'password';
  passwordInput.setAttribute('type', type);

  // Change the text of the toggle button
  this.textContent = type === 'password' ? 'Show' : 'Hide';
});

