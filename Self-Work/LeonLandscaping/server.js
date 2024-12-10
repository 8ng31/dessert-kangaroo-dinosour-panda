// Author: Angel Benavides
//
// Purpose: Create a server for taking customer information and presenting it
// in an administrator account. 

const express = require('express');
const bodyParser = require('body-parser');
const path = require('path');
const app = express();


const mysql = require('mysql2');

// Create a connection to the MySQL database
const connection = mysql.createConnection({
  host: 'localhost',           // MySQL server address (use 'localhost' if it's on your local machine)
  user: 'admin',                // MySQL username
  password: 'password',      // MySQL password
  database: 'company',    // The name of your database
  port: 3306                   // MySQL port (default is 3306)
});

// Middleware to parse form data
app.use(bodyParser.urlencoded({ extended: true }));

// Serve static files from "public_html"
app.use(express.static('public_html'));

// Serve the form
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public_html', 'app/form.html'));
});

app.get('/admin', (req, res) => {
  res.sendFile(path.join(__dirname, 'public_html', 'admin/dashboard.html'));
});

// ======================== Administrator Routes ===========================//
app.get('/admin/login', (req, res) => {
  res.sendFile(path.join(__dirname, 'public_html',  'admin/login.html'));
});


// Get Service requests
app.get('/ServiceRequests', (req, res) => {
  connection.query('SELECT * FROM service_requests', (err, results, fields) => {
    if (err) {
      console.error('Error executing query:', err.stack);
      return res.status(500).json({ error: 'An error occurred while fetching service requests.' });
    }
    //DEBUG console.log(results);
    res.status(200).json(results); // Send the results back to the client as JSON
    
  });
});

// Handle form submission
app.post('/submit', (req, res) => {
  // Destructure all the required fields from the form
  const { name, phone, email, address, city, zip, serviceType, size } = req.body;

  // Log the received data
  console.log(`Name: ${name}, Phone: ${phone}, Email: ${email}, Address: ${address}, City: ${city}, Zip: ${zip}, Service: ${serviceType}, Size: ${size}`);

  // Define the SQL query to insert data into the service_requests table
  const query = `
    INSERT INTO service_requests (name, phone, email, address, city, zip, service_type, area_size)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
  `;

  // The values to be inserted into the database
  const values = [name, phone, email, address, city, zip, serviceType, size];

  // Execute the query to insert the form data into the database
  connection.query(query, values, (err, results) => {
    if (err) {
      console.error('Error inserting data into service_requests:', err.stack);
      return res.status(500).send('Error saving your request, please try again later.');
    }

    console.log('New service request added with ID:', results.insertId);

    // Send a success message to the client
    res.send('Thank you for your request! We will contact you soon.');
  });
});


// Connect to the MySQL database
connection.connect((err) => {
  if (err) {
    console.error('Error connecting to MySQL:', err.stack);
    return;
  }
  console.log('Connected to MySQL as ID', connection.threadId);
  app.listen(3000, () => {
    console.log('Server is running on port 3000');
  });
});

