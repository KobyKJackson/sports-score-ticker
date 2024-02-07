import React from 'react';
import SportsSettings from './Components/SportsSettings';
import GeneralSettings from './Components/GeneralSettings';
import { Container, Typography, Divider } from '@mui/material';

function App() {
    return (
        <Container>
            <Typography variant="h4" gutterBottom>
                Settings
            </Typography>
            <SportsSettings />
            <GeneralSettings /> {/* Render the new GeneralSettings component here */}
        </Container>
    );
}

export default App;
