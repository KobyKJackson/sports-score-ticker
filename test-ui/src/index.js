import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import AppTest from './AppTest';
import { createTheme, ThemeProvider } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';

const theme = createTheme({
  palette: {
    mode: 'dark',
  },
});

const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <ThemeProvider theme={theme}>
    <CssBaseline></CssBaseline>
    <AppTest />
  </ThemeProvider>
);

