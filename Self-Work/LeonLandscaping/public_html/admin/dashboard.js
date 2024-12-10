// Fetch service requests from the API
fetch('/ServiceRequests')
  .then(response => response.json()) // Parse the JSON from the response
  .then(data => {
    const tableBody = document.getElementById('serviceRequestsTable'); // Get the table body

    // Loop through the data and add rows to the table
    data.forEach(request => {
      const row = document.createElement('tr'); // Create a new row

      // Create cells with the request's data
      row.innerHTML = `
        <td>${request.id}</td>
        <td>${request.name}</td>
        <td>${request.phone}</td>
        <td>${request.email}</td>
        <td>${request.address}</td>
        <td>${request.city}</td>
        <td>${request.zip}</td>
        <td>${request.service_type}</td>
        <td>${request.area_size}</td>
      `;

      // Append the new row to the table body
      tableBody.appendChild(row);
    });
  })
  .catch(error => console.error('Error fetching service requests:', error)); // Handle errors

